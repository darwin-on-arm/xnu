/*
 * Copyright (c) 2004-2012 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include "kpi_interface.h"

#include <sys/queue.h>
#include <sys/param.h>	/* for definition of NULL */
#include <kern/debug.h> /* for panic */
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/kern_event.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kpi_mbuf.h>
#include <sys/mcache.h>
#include <sys/protosw.h>
#include <net/if_var.h>
#include <net/if_dl.h>
#include <net/dlil.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_arp.h>
#include <net/if_llreach.h>
#include <net/route.h>
#include <libkern/libkern.h>
#include <libkern/OSAtomic.h>
#include <kern/locks.h>
#include <kern/clock.h>
#include <sys/sockio.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#ifdef INET
#include <netinet/igmp_var.h>
#endif
#ifdef INET6
#include <netinet6/mld6_var.h>
#endif

#include "net/net_str_id.h"

#if IF_LASTCHANGEUPTIME
#define	TOUCHLASTCHANGE(__if_lastchange) {				\
	(__if_lastchange)->tv_sec = net_uptime();			\
	(__if_lastchange)->tv_usec = 0;					\
}
#else
#define	TOUCHLASTCHANGE(__if_lastchange) microtime(__if_lastchange)
#endif

#define	_cast_non_const(p) ((void *)(uintptr_t)(p))

static errno_t ifnet_defrouter_llreachinfo(ifnet_t, int,
    struct ifnet_llreach_info *);
static void ifnet_kpi_free(ifnet_t);
static errno_t ifnet_list_get_common(ifnet_family_t, boolean_t, ifnet_t **,
    u_int32_t *);
static errno_t ifnet_set_lladdr_internal(ifnet_t, const void *, size_t,
    u_char, int);
static errno_t ifnet_awdl_check_eflags(ifnet_t, u_int32_t *, u_int32_t *);

/*
 * Temporary work around until we have real reference counting
 *
 * We keep the bits about calling dlil_if_release (which should be
 * called recycle) transparent by calling it from our if_free function
 * pointer. We have to keep the client's original detach function
 * somewhere so we can call it.
 */
static void
ifnet_kpi_free(ifnet_t ifp)
{
	ifnet_detached_func detach_func = ifp->if_kpi_storage;

	if (detach_func != NULL)
		detach_func(ifp);

	if (ifp->if_broadcast.length > sizeof (ifp->if_broadcast.u.buffer)) {
		FREE(ifp->if_broadcast.u.ptr, M_IFADDR);
		ifp->if_broadcast.u.ptr = NULL;
	}

	dlil_if_release(ifp);
}

errno_t
ifnet_allocate(const struct ifnet_init_params *init, ifnet_t *interface)
{
	struct ifnet_init_eparams einit;

	bzero(&einit, sizeof (einit));

	einit.ver		= IFNET_INIT_CURRENT_VERSION;
	einit.len		= sizeof (einit);
	einit.flags		= IFNET_INIT_LEGACY;
	einit.uniqueid		= init->uniqueid;
	einit.uniqueid_len	= init->uniqueid_len;
	einit.name		= init->name;
	einit.unit		= init->unit;
	einit.family		= init->family;
	einit.type		= init->type;
	einit.output		= init->output;
	einit.demux		= init->demux;
	einit.add_proto		= init->add_proto;
	einit.del_proto		= init->del_proto;
	einit.check_multi	= init->check_multi;
	einit.framer		= init->framer;
	einit.softc		= init->softc;
	einit.ioctl		= init->ioctl;
	einit.set_bpf_tap	= init->set_bpf_tap;
	einit.detach		= init->detach;
	einit.event		= init->event;
	einit.broadcast_addr	= init->broadcast_addr;
	einit.broadcast_len	= init->broadcast_len;

	return (ifnet_allocate_extended(&einit, interface));
}

errno_t
ifnet_allocate_extended(const struct ifnet_init_eparams *einit0,
    ifnet_t *interface)
{
	struct ifnet_init_eparams einit;
	struct ifnet *ifp = NULL;
	int error;

	einit = *einit0;

	if (einit.ver != IFNET_INIT_CURRENT_VERSION ||
	    einit.len < sizeof (einit))
		return (EINVAL);

	if (einit.family == 0 || einit.name == NULL ||
	    strlen(einit.name) >= IFNAMSIZ ||
	    (einit.type & 0xFFFFFF00) != 0 || einit.type == 0)
		return (EINVAL);

	if (einit.flags & IFNET_INIT_LEGACY) {
		if (einit.output == NULL || einit.flags != IFNET_INIT_LEGACY)
			return (EINVAL);

		einit.pre_enqueue = NULL;
		einit.start = NULL;
		einit.output_ctl = NULL;
		einit.output_sched_model = IFNET_SCHED_MODEL_NORMAL;
		einit.input_poll = NULL;
		einit.input_ctl = NULL;
	} else {
		if (einit.start == NULL)
			return (EINVAL);

		einit.output = NULL;
		if (einit.output_sched_model >= IFNET_SCHED_MODEL_MAX)
			return (EINVAL);

		if (einit.flags & IFNET_INIT_INPUT_POLL) {
			if (einit.input_poll == NULL || einit.input_ctl == NULL)
				return (EINVAL);
		} else {
			einit.input_poll = NULL;
			einit.input_ctl = NULL;
		}
	}

	error = dlil_if_acquire(einit.family, einit.uniqueid,
	    einit.uniqueid_len, &ifp);

	if (error == 0) {
		u_int64_t br;

		/*
		 * Cast ifp->if_name as non const. dlil_if_acquire sets it up
		 * to point to storage of at least IFNAMSIZ bytes. It is safe
		 * to write to this.
		 */
		strncpy(_cast_non_const(ifp->if_name), einit.name, IFNAMSIZ);
		ifp->if_type		= einit.type;
		ifp->if_family		= einit.family;
		ifp->if_unit		= einit.unit;
		ifp->if_output		= einit.output;
		ifp->if_pre_enqueue	= einit.pre_enqueue;
		ifp->if_start		= einit.start;
		ifp->if_output_ctl	= einit.output_ctl;
		ifp->if_output_sched_model = einit.output_sched_model;
		ifp->if_output_bw.eff_bw = einit.output_bw;
		ifp->if_output_bw.max_bw = einit.output_bw_max;
		ifp->if_input_poll	= einit.input_poll;
		ifp->if_input_ctl	= einit.input_ctl;
		ifp->if_input_bw.eff_bw	= einit.input_bw;
		ifp->if_input_bw.max_bw	= einit.input_bw_max;
		ifp->if_demux		= einit.demux;
		ifp->if_add_proto	= einit.add_proto;
		ifp->if_del_proto	= einit.del_proto;
		ifp->if_check_multi	= einit.check_multi;
		ifp->if_framer		= einit.framer;
		ifp->if_softc		= einit.softc;
		ifp->if_ioctl		= einit.ioctl;
		ifp->if_set_bpf_tap	= einit.set_bpf_tap;
		ifp->if_free		= ifnet_kpi_free;
		ifp->if_event		= einit.event;
		ifp->if_kpi_storage	= einit.detach;

		if (ifp->if_output_bw.eff_bw > ifp->if_output_bw.max_bw)
			ifp->if_output_bw.max_bw = ifp->if_output_bw.eff_bw;
		else if (ifp->if_output_bw.eff_bw == 0)
			ifp->if_output_bw.eff_bw = ifp->if_output_bw.max_bw;

		if (ifp->if_input_bw.eff_bw > ifp->if_input_bw.max_bw)
			ifp->if_input_bw.max_bw = ifp->if_input_bw.eff_bw;
		else if (ifp->if_input_bw.eff_bw == 0)
			ifp->if_input_bw.eff_bw = ifp->if_input_bw.max_bw;

		if (ifp->if_output_bw.max_bw == 0)
			ifp->if_output_bw = ifp->if_input_bw;
		else if (ifp->if_input_bw.max_bw == 0)
			ifp->if_input_bw = ifp->if_output_bw;

		if (ifp->if_ioctl == NULL)
			ifp->if_ioctl = ifp_if_ioctl;

		/* Pin if_baudrate to 32 bits */
		br = MAX(ifp->if_output_bw.max_bw, ifp->if_input_bw.max_bw);
		if (br != 0)
			ifp->if_baudrate = (br > 0xFFFFFFFF) ? 0xFFFFFFFF : br;

		if (ifp->if_start != NULL) {
			ifp->if_eflags |= IFEF_TXSTART;
			if (ifp->if_pre_enqueue == NULL)
				ifp->if_pre_enqueue = ifnet_enqueue;
			ifp->if_output = ifp->if_pre_enqueue;
		} else {
			ifp->if_eflags &= ~IFEF_TXSTART;
		}

		if (ifp->if_input_poll != NULL)
			ifp->if_eflags |= IFEF_RXPOLL;
		else
			ifp->if_eflags &= ~IFEF_RXPOLL;

		VERIFY(!(einit.flags & IFNET_INIT_LEGACY) ||
		    (ifp->if_pre_enqueue == NULL && ifp->if_start == NULL &&
		    ifp->if_output_ctl == NULL && ifp->if_input_poll == NULL &&
		    ifp->if_input_ctl == NULL));
		VERIFY(!(einit.flags & IFNET_INIT_INPUT_POLL) ||
		    (ifp->if_input_poll != NULL && ifp->if_input_ctl != NULL));

		if (einit.broadcast_len && einit.broadcast_addr) {
			if (einit.broadcast_len >
			    sizeof (ifp->if_broadcast.u.buffer)) {
				MALLOC(ifp->if_broadcast.u.ptr, u_char *,
				    einit.broadcast_len, M_IFADDR, M_NOWAIT);
				if (ifp->if_broadcast.u.ptr == NULL) {
					error = ENOMEM;
				} else {
					bcopy(einit.broadcast_addr,
					    ifp->if_broadcast.u.ptr,
					    einit.broadcast_len);
				}
			} else {
				bcopy(einit.broadcast_addr,
				    ifp->if_broadcast.u.buffer,
				    einit.broadcast_len);
			}
			ifp->if_broadcast.length = einit.broadcast_len;
		} else {
			bzero(&ifp->if_broadcast, sizeof (ifp->if_broadcast));
		}

		IFCQ_MAXLEN(&ifp->if_snd) = einit.sndq_maxlen;

		if (error == 0) {
			*interface = ifp;
			// temporary - this should be done in dlil_if_acquire
			ifnet_reference(ifp);
		} else {
			dlil_if_release(ifp);
			*interface = NULL;
		}
	}

	/*
	 * Note: We should do something here to indicate that we haven't been
	 * attached yet. By doing so, we can catch the case in ifnet_release
	 * where the reference count reaches zero and call the recycle
	 * function. If the interface is attached, the interface will be
	 * recycled when the interface's if_free function is called. If the
	 * interface is never attached, the if_free function will never be
	 * called and the interface will never be recycled.
	 */

	return (error);
}

errno_t
ifnet_reference(ifnet_t ifp)
{
	return (dlil_if_ref(ifp));
}

errno_t
ifnet_release(ifnet_t ifp)
{
	return (dlil_if_free(ifp));
}

errno_t
ifnet_interface_family_find(const char *module_string,
    ifnet_family_t *family_id)
{
	if (module_string == NULL || family_id == NULL)
		return (EINVAL);

	return (net_str_id_find_internal(module_string, family_id,
	    NSI_IF_FAM_ID, 1));
}

void *
ifnet_softc(ifnet_t interface)
{
	return ((interface == NULL) ? NULL : interface->if_softc);
}

const char *
ifnet_name(ifnet_t interface)
{
	return ((interface == NULL) ? NULL : interface->if_name);
}

ifnet_family_t
ifnet_family(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_family);
}

u_int32_t
ifnet_unit(ifnet_t interface)
{
	return ((interface == NULL) ? (u_int32_t)0xffffffff :
	    (u_int32_t)interface->if_unit);
}

u_int32_t
ifnet_index(ifnet_t interface)
{
	return ((interface == NULL) ? (u_int32_t)0xffffffff :
	    interface->if_index);
}

errno_t
ifnet_set_flags(ifnet_t interface, u_int16_t new_flags, u_int16_t mask)
{
	uint16_t old_flags;

	if (interface == NULL)
		return (EINVAL);

	ifnet_lock_exclusive(interface);

	/* If we are modifying the up/down state, call if_updown */
	if ((mask & IFF_UP) != 0) {
		if_updown(interface, (new_flags & IFF_UP) == IFF_UP);
	}

	old_flags = interface->if_flags;
	interface->if_flags = (new_flags & mask) | (interface->if_flags & ~mask);
	/* If we are modifying the multicast flag, set/unset the silent flag */
	if ((old_flags & IFF_MULTICAST) !=
	    (interface->if_flags & IFF_MULTICAST)) {
#if INET
		if (IGMP_IFINFO(interface) != NULL)
			igmp_initsilent(interface, IGMP_IFINFO(interface));
#endif /* INET */
#if INET6
		if (MLD_IFINFO(interface) != NULL)
			mld6_initsilent(interface, MLD_IFINFO(interface));
#endif /* INET6 */
	}

	ifnet_lock_done(interface);

	return (0);
}

u_int16_t
ifnet_flags(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_flags);
}

/*
 * This routine ensures the following:
 *
 * If IFEF_AWDL is set by the caller, also set the rest of flags as
 * defined in IFEF_AWDL_MASK.
 *
 * If IFEF_AWDL has been set on the interface and the caller attempts
 * to clear one or more of the associated flags in IFEF_AWDL_MASK,
 * return failure.
 *
 * All other flags not associated with AWDL are not affected.
 *
 * See <net/if.h> for current definition of IFEF_AWDL_MASK.
 */
static errno_t
ifnet_awdl_check_eflags(ifnet_t ifp, u_int32_t *new_eflags, u_int32_t *mask)
{
	u_int32_t eflags;

	ifnet_lock_assert(ifp, IFNET_LCK_ASSERT_EXCLUSIVE);

	eflags = (*new_eflags & *mask) | (ifp->if_eflags & ~(*mask));

	if (ifp->if_eflags & IFEF_AWDL) {
		if (eflags & IFEF_AWDL) {
			if ((eflags & IFEF_AWDL_MASK) != IFEF_AWDL_MASK)
				return (1);
		} else {
			*new_eflags &= ~IFEF_AWDL_MASK;
			*mask |= IFEF_AWDL_MASK;
		}
	} else if (eflags & IFEF_AWDL) {
		*new_eflags |= IFEF_AWDL_MASK;
		*mask |= IFEF_AWDL_MASK;
	}

	return (0);
}

errno_t
ifnet_set_eflags(ifnet_t interface, u_int32_t new_flags, u_int32_t mask)
{
	if (interface == NULL)
		return (EINVAL);

	ifnet_lock_exclusive(interface);
	/*
	 * Sanity checks for IFEF_AWDL and its related flags.
	 */
	if (ifnet_awdl_check_eflags(interface, &new_flags, &mask) != 0) {
		ifnet_lock_done(interface);
		return (EINVAL);
	}
	interface->if_eflags =
	    (new_flags & mask) | (interface->if_eflags & ~mask);
	ifnet_lock_done(interface);

	return (0);
}

u_int32_t
ifnet_eflags(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_eflags);
}

errno_t
ifnet_set_idle_flags_locked(ifnet_t ifp, u_int32_t new_flags, u_int32_t mask)
{
	int before, after;

	if (ifp == NULL)
		return (EINVAL);

	lck_mtx_assert(rnh_lock, LCK_MTX_ASSERT_OWNED);
	ifnet_lock_assert(ifp, IFNET_LCK_ASSERT_EXCLUSIVE);

	/*
	 * If this is called prior to ifnet attach, the actual work will
	 * be done at attach time.  Otherwise, if it is called after
	 * ifnet detach, then it is a no-op.
	 */
	if (!ifnet_is_attached(ifp, 0)) {
		ifp->if_idle_new_flags = new_flags;
		ifp->if_idle_new_flags_mask = mask;
		return (0);
	} else {
		ifp->if_idle_new_flags = ifp->if_idle_new_flags_mask = 0;
	}

	before = ifp->if_idle_flags;
	ifp->if_idle_flags = (new_flags & mask) | (ifp->if_idle_flags & ~mask);
	after = ifp->if_idle_flags;

	if ((after - before) < 0 && ifp->if_idle_flags == 0 &&
	    ifp->if_want_aggressive_drain != 0) {
		ifp->if_want_aggressive_drain = 0;
		if (ifnet_aggressive_drainers == 0)
			panic("%s: ifp=%p negative aggdrain!", __func__, ifp);
		if (--ifnet_aggressive_drainers == 0)
			rt_aggdrain(0);
	} else if ((after - before) > 0 && ifp->if_want_aggressive_drain == 0) {
		ifp->if_want_aggressive_drain++;
		if (++ifnet_aggressive_drainers == 0)
			panic("%s: ifp=%p wraparound aggdrain!", __func__, ifp);
		else if (ifnet_aggressive_drainers == 1)
			rt_aggdrain(1);
	}

	return (0);
}

errno_t
ifnet_set_idle_flags(ifnet_t ifp, u_int32_t new_flags, u_int32_t mask)
{
	errno_t err;

	lck_mtx_lock(rnh_lock);
	ifnet_lock_exclusive(ifp);
	err = ifnet_set_idle_flags_locked(ifp, new_flags, mask);
	ifnet_lock_done(ifp);
	lck_mtx_unlock(rnh_lock);

	return (err);
}

u_int32_t
ifnet_idle_flags(ifnet_t ifp)
{
	return ((ifp == NULL) ? 0 : ifp->if_idle_flags);
}

errno_t
ifnet_set_link_quality(ifnet_t ifp, int quality)
{
	errno_t err = 0;

	if (ifp == NULL || quality < IFNET_LQM_MIN || quality > IFNET_LQM_MAX) {
		err = EINVAL;
		goto done;
	}

	if (!ifnet_is_attached(ifp, 0)) {
		err = ENXIO;
		goto done;
	}

	if_lqm_update(ifp, quality);

done:
	return (err);
}

int
ifnet_link_quality(ifnet_t ifp)
{
	int lqm;

	if (ifp == NULL)
		return (IFNET_LQM_THRESH_OFF);

	ifnet_lock_shared(ifp);
	lqm = ifp->if_lqm;
	ifnet_lock_done(ifp);

	return (lqm);
}

static errno_t
ifnet_defrouter_llreachinfo(ifnet_t ifp, int af,
    struct ifnet_llreach_info *iflri)
{
	if (ifp == NULL || iflri == NULL)
		return (EINVAL);

	VERIFY(af == AF_INET || af == AF_INET6);

	return (ifnet_llreach_get_defrouter(ifp, af, iflri));
}

errno_t
ifnet_inet_defrouter_llreachinfo(ifnet_t ifp, struct ifnet_llreach_info *iflri)
{
	return (ifnet_defrouter_llreachinfo(ifp, AF_INET, iflri));
}

errno_t
ifnet_inet6_defrouter_llreachinfo(ifnet_t ifp, struct ifnet_llreach_info *iflri)
{
	return (ifnet_defrouter_llreachinfo(ifp, AF_INET6, iflri));
}

errno_t
ifnet_set_capabilities_supported(ifnet_t ifp, u_int32_t new_caps,
    u_int32_t mask)
{
	errno_t error = 0;
	int tmp;

	if (ifp == NULL)
		return (EINVAL);

	ifnet_lock_exclusive(ifp);
	tmp = (new_caps & mask) | (ifp->if_capabilities & ~mask);
	if ((tmp & ~IFCAP_VALID))
		error = EINVAL;
	else
		ifp->if_capabilities = tmp;
	ifnet_lock_done(ifp);

	return (error);
}

u_int32_t
ifnet_capabilities_supported(ifnet_t ifp)
{
	return ((ifp == NULL) ? 0 : ifp->if_capabilities);
}


errno_t
ifnet_set_capabilities_enabled(ifnet_t ifp, u_int32_t new_caps,
    u_int32_t mask)
{
	errno_t error = 0;
	int tmp;
	struct kev_msg ev_msg;
	struct net_event_data ev_data;

	if (ifp == NULL)
		return (EINVAL);

	ifnet_lock_exclusive(ifp);
	tmp = (new_caps & mask) | (ifp->if_capenable & ~mask);
	if ((tmp & ~IFCAP_VALID) || (tmp & ~ifp->if_capabilities))
		error = EINVAL;
	else
		ifp->if_capenable = tmp;
	ifnet_lock_done(ifp);

	/* Notify application of the change */
	bzero(&ev_data, sizeof (struct net_event_data));
	bzero(&ev_msg, sizeof (struct kev_msg));
	ev_msg.vendor_code	= KEV_VENDOR_APPLE;
	ev_msg.kev_class	= KEV_NETWORK_CLASS;
	ev_msg.kev_subclass	= KEV_DL_SUBCLASS;

	ev_msg.event_code	= KEV_DL_IFCAP_CHANGED;
	strlcpy(&ev_data.if_name[0], ifp->if_name, IFNAMSIZ);
	ev_data.if_family	= ifp->if_family;
	ev_data.if_unit		= (u_int32_t)ifp->if_unit;
	ev_msg.dv[0].data_length = sizeof (struct net_event_data);
	ev_msg.dv[0].data_ptr = &ev_data;
	ev_msg.dv[1].data_length = 0;
	kev_post_msg(&ev_msg);

	return (error);
}

u_int32_t
ifnet_capabilities_enabled(ifnet_t ifp)
{
	return ((ifp == NULL) ? 0 : ifp->if_capenable);
}

static const ifnet_offload_t offload_mask =
	(IFNET_CSUM_IP | IFNET_CSUM_TCP | IFNET_CSUM_UDP | IFNET_CSUM_FRAGMENT |
	IFNET_IP_FRAGMENT | IFNET_CSUM_TCPIPV6 | IFNET_CSUM_UDPIPV6 |
	IFNET_IPV6_FRAGMENT | IFNET_CSUM_SUM16 | IFNET_VLAN_TAGGING |
	IFNET_VLAN_MTU | IFNET_MULTIPAGES | IFNET_TSO_IPV4 | IFNET_TSO_IPV6);

static const ifnet_offload_t any_offload_csum =
	(IFNET_CSUM_IP | IFNET_CSUM_TCP | IFNET_CSUM_UDP | IFNET_CSUM_FRAGMENT |
	IFNET_CSUM_TCPIPV6 | IFNET_CSUM_UDPIPV6 | IFNET_CSUM_SUM16);

errno_t
ifnet_set_offload(ifnet_t interface, ifnet_offload_t offload)
{
	u_int32_t ifcaps = 0;

	if (interface == NULL)
		return (EINVAL);

	ifnet_lock_exclusive(interface);
	interface->if_hwassist = (offload & offload_mask);
	ifnet_lock_done(interface);

	if ((offload & any_offload_csum))
		ifcaps |= IFCAP_HWCSUM;
	if ((offload & IFNET_TSO_IPV4))
		ifcaps |= IFCAP_TSO4;
	if ((offload & IFNET_TSO_IPV6))
		ifcaps |= IFCAP_TSO6;
	if ((offload & IFNET_VLAN_MTU))
		ifcaps |= IFCAP_VLAN_MTU;
	if ((offload & IFNET_VLAN_TAGGING))
		ifcaps |= IFCAP_VLAN_HWTAGGING;
	if (ifcaps != 0) {
		(void) ifnet_set_capabilities_supported(interface, ifcaps,
		    IFCAP_VALID);
		(void) ifnet_set_capabilities_enabled(interface, ifcaps,
		    IFCAP_VALID);
	}

	return (0);
}

ifnet_offload_t
ifnet_offload(ifnet_t interface)
{
	return ((interface == NULL) ?
	    0 : (interface->if_hwassist & offload_mask));
}

errno_t
ifnet_set_tso_mtu(ifnet_t interface, sa_family_t family, u_int32_t mtuLen)
{
	errno_t error = 0;

	if (interface == NULL || mtuLen < interface->if_mtu)
		return (EINVAL);

	switch (family) {
	case AF_INET:
		if (interface->if_hwassist & IFNET_TSO_IPV4)
			interface->if_tso_v4_mtu = mtuLen;
		else
			error = EINVAL;
		break;

	case AF_INET6:
		if (interface->if_hwassist & IFNET_TSO_IPV6)
			interface->if_tso_v6_mtu = mtuLen;
		else
			error = EINVAL;
		break;

	default:
		error = EPROTONOSUPPORT;
		break;
	}

	return (error);
}

errno_t
ifnet_get_tso_mtu(ifnet_t interface, sa_family_t family, u_int32_t *mtuLen)
{
	errno_t error = 0;

	if (interface == NULL || mtuLen == NULL)
		return (EINVAL);

	switch (family) {
	case AF_INET:
		if (interface->if_hwassist & IFNET_TSO_IPV4)
			*mtuLen = interface->if_tso_v4_mtu;
		else
			error = EINVAL;
		break;

	case AF_INET6:
		if (interface->if_hwassist & IFNET_TSO_IPV6)
			*mtuLen = interface->if_tso_v6_mtu;
		else
			error = EINVAL;
		break;

	default:
		error = EPROTONOSUPPORT;
		break;
	}

	return (error);
}

errno_t
ifnet_set_wake_flags(ifnet_t interface, u_int32_t properties, u_int32_t mask)
{
	struct kev_msg ev_msg;
	struct net_event_data ev_data;

	bzero(&ev_data, sizeof (struct net_event_data));
	bzero(&ev_msg, sizeof (struct kev_msg));

	if (interface == NULL)
		return (EINVAL);

	/* Do not accept wacky values */
	if ((properties & mask) & ~IF_WAKE_VALID_FLAGS)
		return (EINVAL);

	ifnet_lock_exclusive(interface);

	interface->if_wake_properties =
	    (properties & mask) | (interface->if_wake_properties & ~mask);

	ifnet_lock_done(interface);

	(void) ifnet_touch_lastchange(interface);

	/* Notify application of the change */
	ev_msg.vendor_code	= KEV_VENDOR_APPLE;
	ev_msg.kev_class	= KEV_NETWORK_CLASS;
	ev_msg.kev_subclass	= KEV_DL_SUBCLASS;

	ev_msg.event_code	= KEV_DL_WAKEFLAGS_CHANGED;
	strlcpy(&ev_data.if_name[0], interface->if_name, IFNAMSIZ);
	ev_data.if_family	= interface->if_family;
	ev_data.if_unit		= (u_int32_t)interface->if_unit;
	ev_msg.dv[0].data_length = sizeof (struct net_event_data);
	ev_msg.dv[0].data_ptr	= &ev_data;
	ev_msg.dv[1].data_length = 0;
	kev_post_msg(&ev_msg);

	return (0);
}

u_int32_t
ifnet_get_wake_flags(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_wake_properties);
}

/*
 * Should MIB data store a copy?
 */
errno_t
ifnet_set_link_mib_data(ifnet_t interface, void *mibData, u_int32_t mibLen)
{
	if (interface == NULL)
		return (EINVAL);

	ifnet_lock_exclusive(interface);
	interface->if_linkmib = (void*)mibData;
	interface->if_linkmiblen = mibLen;
	ifnet_lock_done(interface);
	return (0);
}

errno_t
ifnet_get_link_mib_data(ifnet_t interface, void *mibData, u_int32_t *mibLen)
{
	errno_t	result = 0;

	if (interface == NULL)
		return (EINVAL);

	ifnet_lock_shared(interface);
	if (*mibLen < interface->if_linkmiblen)
		result = EMSGSIZE;
	if (result == 0 && interface->if_linkmib == NULL)
		result = ENOTSUP;

	if (result == 0) {
		*mibLen = interface->if_linkmiblen;
		bcopy(interface->if_linkmib, mibData, *mibLen);
	}
	ifnet_lock_done(interface);

	return (result);
}

u_int32_t
ifnet_get_link_mib_data_length(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_linkmiblen);
}

errno_t
ifnet_output(ifnet_t interface, protocol_family_t protocol_family,
    mbuf_t m, void *route, const struct sockaddr *dest)
{
	if (interface == NULL || protocol_family == 0 || m == NULL) {
		if (m != NULL)
			mbuf_freem_list(m);
		return (EINVAL);
	}
	return (dlil_output(interface, protocol_family, m, route, dest, 0, NULL));
}

errno_t
ifnet_output_raw(ifnet_t interface, protocol_family_t protocol_family, mbuf_t m)
{
	if (interface == NULL || m == NULL) {
		if (m != NULL)
			mbuf_freem_list(m);
		return (EINVAL);
	}
	return (dlil_output(interface, protocol_family, m, NULL, NULL, 1, NULL));
}

errno_t
ifnet_set_mtu(ifnet_t interface, u_int32_t mtu)
{
	if (interface == NULL)
		return (EINVAL);

	interface->if_mtu = mtu;
	return (0);
}

u_int32_t
ifnet_mtu(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_mtu);
}

u_char
ifnet_type(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_data.ifi_type);
}

errno_t
ifnet_set_addrlen(ifnet_t interface, u_char addrlen)
{
	if (interface == NULL)
		return (EINVAL);

	interface->if_data.ifi_addrlen = addrlen;
	return (0);
}

u_char
ifnet_addrlen(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_data.ifi_addrlen);
}

errno_t
ifnet_set_hdrlen(ifnet_t interface, u_char hdrlen)
{
	if (interface == NULL)
		return (EINVAL);

	interface->if_data.ifi_hdrlen = hdrlen;
	return (0);
}

u_char
ifnet_hdrlen(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_data.ifi_hdrlen);
}

errno_t
ifnet_set_metric(ifnet_t interface, u_int32_t metric)
{
	if (interface == NULL)
		return (EINVAL);

	interface->if_data.ifi_metric = metric;
	return (0);
}

u_int32_t
ifnet_metric(ifnet_t interface)
{
	return ((interface == NULL) ? 0 : interface->if_data.ifi_metric);
}

errno_t
ifnet_set_baudrate(struct ifnet *ifp, u_int64_t baudrate)
{
	if (ifp == NULL)
		return (EINVAL);

	ifp->if_output_bw.max_bw = ifp->if_input_bw.max_bw =
	    ifp->if_output_bw.eff_bw = ifp->if_input_bw.eff_bw = baudrate;

	/* Pin if_baudrate to 32 bits until we can change the storage size */
	ifp->if_baudrate = (baudrate > 0xFFFFFFFF) ? 0xFFFFFFFF : baudrate;

	return (0);
}

u_int64_t
ifnet_baudrate(struct ifnet *ifp)
{
	return ((ifp == NULL) ? 0 : ifp->if_baudrate);
}

errno_t
ifnet_set_bandwidths(struct ifnet *ifp, struct if_bandwidths *output_bw,
    struct if_bandwidths *input_bw)
{
	if (ifp == NULL)
		return (EINVAL);

	if (input_bw != NULL)
		(void) ifnet_set_input_bandwidths(ifp, input_bw);

	if (output_bw != NULL)
		(void) ifnet_set_output_bandwidths(ifp, output_bw, FALSE);

	return (0);
}

errno_t
ifnet_set_output_bandwidths(struct ifnet *ifp, struct if_bandwidths *bw,
    boolean_t locked)
{
	struct if_bandwidths old_bw;
	struct ifclassq *ifq;
	u_int64_t br;

	ifq = &ifp->if_snd;
	if (!locked)
		IFCQ_LOCK(ifq);
	IFCQ_LOCK_ASSERT_HELD(ifq);

	old_bw = ifp->if_output_bw;
	if (bw != NULL) {
		if (bw->eff_bw != 0)
			ifp->if_output_bw.eff_bw = bw->eff_bw;
		if (bw->max_bw != 0)
			ifp->if_output_bw.max_bw = bw->max_bw;
		if (ifp->if_output_bw.eff_bw > ifp->if_output_bw.max_bw)
			ifp->if_output_bw.max_bw = ifp->if_output_bw.eff_bw;
		else if (ifp->if_output_bw.eff_bw == 0)
			ifp->if_output_bw.eff_bw = ifp->if_output_bw.max_bw;
	}

	/* Pin if_baudrate to 32 bits */
	br = MAX(ifp->if_output_bw.max_bw, ifp->if_input_bw.max_bw);
	if (br != 0)
		ifp->if_baudrate = (br > 0xFFFFFFFF) ? 0xFFFFFFFF : br;

	/* Adjust queue parameters if needed */
	if (old_bw.eff_bw != ifp->if_output_bw.eff_bw ||
	    old_bw.max_bw != ifp->if_output_bw.max_bw)
		ifnet_update_sndq(ifq, CLASSQ_EV_LINK_SPEED);

	if (!locked)
		IFCQ_UNLOCK(ifq);

	return (0);
}

errno_t
ifnet_set_input_bandwidths(struct ifnet *ifp, struct if_bandwidths *bw)
{
	struct if_bandwidths old_bw;

	old_bw = ifp->if_input_bw;
	if (bw->eff_bw != 0)
		ifp->if_input_bw.eff_bw = bw->eff_bw;
	if (bw->max_bw != 0)
		ifp->if_input_bw.max_bw = bw->max_bw;
	if (ifp->if_input_bw.eff_bw > ifp->if_input_bw.max_bw)
		ifp->if_input_bw.max_bw = ifp->if_input_bw.eff_bw;
	else if (ifp->if_input_bw.eff_bw == 0)
		ifp->if_input_bw.eff_bw = ifp->if_input_bw.max_bw;

	if (old_bw.eff_bw != ifp->if_input_bw.eff_bw ||
	    old_bw.max_bw != ifp->if_input_bw.max_bw)
		ifnet_update_rcv(ifp, CLASSQ_EV_LINK_SPEED);

	return (0);
}

u_int64_t
ifnet_output_linkrate(struct ifnet *ifp)
{
	struct ifclassq *ifq = &ifp->if_snd;
	u_int64_t rate;

	IFCQ_LOCK_ASSERT_HELD(ifq);

	rate = ifp->if_output_bw.eff_bw;
	if (IFCQ_TBR_IS_ENABLED(ifq)) {
		u_int64_t tbr_rate = ifp->if_snd.ifcq_tbr.tbr_rate_raw;
		VERIFY(tbr_rate > 0);
		rate = MIN(rate, ifp->if_snd.ifcq_tbr.tbr_rate_raw);
	}

	return (rate);
}

u_int64_t
ifnet_input_linkrate(struct ifnet *ifp)
{
	return (ifp->if_input_bw.eff_bw);
}

errno_t
ifnet_bandwidths(struct ifnet *ifp, struct if_bandwidths *output_bw,
    struct if_bandwidths *input_bw)
{
	if (ifp == NULL)
		return (EINVAL);

	if (output_bw != NULL)
		*output_bw = ifp->if_output_bw;
	if (input_bw != NULL)
		*input_bw = ifp->if_input_bw;

	return (0);
}

errno_t
ifnet_stat_increment(struct ifnet *ifp,
    const struct ifnet_stat_increment_param *s)
{
	if (ifp == NULL)
		return (EINVAL);

	if (s->packets_in != 0)
		atomic_add_64(&ifp->if_data.ifi_ipackets, s->packets_in);
	if (s->bytes_in != 0)
		atomic_add_64(&ifp->if_data.ifi_ibytes, s->bytes_in);
	if (s->errors_in != 0)
		atomic_add_64(&ifp->if_data.ifi_ierrors, s->errors_in);

	if (s->packets_out != 0)
		atomic_add_64(&ifp->if_data.ifi_opackets, s->packets_out);
	if (s->bytes_out != 0)
		atomic_add_64(&ifp->if_data.ifi_obytes, s->bytes_out);
	if (s->errors_out != 0)
		atomic_add_64(&ifp->if_data.ifi_oerrors, s->errors_out);

	if (s->collisions != 0)
		atomic_add_64(&ifp->if_data.ifi_collisions, s->collisions);
	if (s->dropped != 0)
		atomic_add_64(&ifp->if_data.ifi_iqdrops, s->dropped);

	/* Touch the last change time. */
	TOUCHLASTCHANGE(&ifp->if_lastchange);

	return (0);
}

errno_t
ifnet_stat_increment_in(struct ifnet *ifp, u_int32_t packets_in,
    u_int32_t bytes_in, u_int32_t errors_in)
{
	if (ifp == NULL)
		return (EINVAL);

	if (packets_in != 0)
		atomic_add_64(&ifp->if_data.ifi_ipackets, packets_in);
	if (bytes_in != 0)
		atomic_add_64(&ifp->if_data.ifi_ibytes, bytes_in);
	if (errors_in != 0)
		atomic_add_64(&ifp->if_data.ifi_ierrors, errors_in);

	TOUCHLASTCHANGE(&ifp->if_lastchange);

	return (0);
}

errno_t
ifnet_stat_increment_out(struct ifnet *ifp, u_int32_t packets_out,
    u_int32_t bytes_out, u_int32_t errors_out)
{
	if (ifp == NULL)
		return (EINVAL);

	if (packets_out != 0)
		atomic_add_64(&ifp->if_data.ifi_opackets, packets_out);
	if (bytes_out != 0)
		atomic_add_64(&ifp->if_data.ifi_obytes, bytes_out);
	if (errors_out != 0)
		atomic_add_64(&ifp->if_data.ifi_oerrors, errors_out);

	TOUCHLASTCHANGE(&ifp->if_lastchange);

	return (0);
}

errno_t
ifnet_set_stat(struct ifnet *ifp, const struct ifnet_stats_param *s)
{
	if (ifp == NULL)
		return (EINVAL);

	atomic_set_64(&ifp->if_data.ifi_ipackets, s->packets_in);
	atomic_set_64(&ifp->if_data.ifi_ibytes, s->bytes_in);
	atomic_set_64(&ifp->if_data.ifi_imcasts, s->multicasts_in);
	atomic_set_64(&ifp->if_data.ifi_ierrors, s->errors_in);

	atomic_set_64(&ifp->if_data.ifi_opackets, s->packets_out);
	atomic_set_64(&ifp->if_data.ifi_obytes, s->bytes_out);
	atomic_set_64(&ifp->if_data.ifi_omcasts, s->multicasts_out);
	atomic_set_64(&ifp->if_data.ifi_oerrors, s->errors_out);

	atomic_set_64(&ifp->if_data.ifi_collisions, s->collisions);
	atomic_set_64(&ifp->if_data.ifi_iqdrops, s->dropped);
	atomic_set_64(&ifp->if_data.ifi_noproto, s->no_protocol);

	/* Touch the last change time. */
	TOUCHLASTCHANGE(&ifp->if_lastchange);

	return (0);
}

errno_t
ifnet_stat(struct ifnet *ifp, struct ifnet_stats_param *s)
{
	if (ifp == NULL)
		return (EINVAL);

	atomic_get_64(s->packets_in, &ifp->if_data.ifi_ipackets);
	atomic_get_64(s->bytes_in, &ifp->if_data.ifi_ibytes);
	atomic_get_64(s->multicasts_in, &ifp->if_data.ifi_imcasts);
	atomic_get_64(s->errors_in, &ifp->if_data.ifi_ierrors);

	atomic_get_64(s->packets_out, &ifp->if_data.ifi_opackets);
	atomic_get_64(s->bytes_out, &ifp->if_data.ifi_obytes);
	atomic_get_64(s->multicasts_out, &ifp->if_data.ifi_omcasts);
	atomic_get_64(s->errors_out, &ifp->if_data.ifi_oerrors);

	atomic_get_64(s->collisions, &ifp->if_data.ifi_collisions);
	atomic_get_64(s->dropped, &ifp->if_data.ifi_iqdrops);
	atomic_get_64(s->no_protocol, &ifp->if_data.ifi_noproto);

	return (0);
}

errno_t
ifnet_touch_lastchange(ifnet_t interface)
{
	if (interface == NULL)
		return (EINVAL);

	TOUCHLASTCHANGE(&interface->if_lastchange);

	return (0);
}

errno_t
ifnet_lastchange(ifnet_t interface, struct timeval *last_change)
{
	if (interface == NULL)
		return (EINVAL);

	*last_change = interface->if_data.ifi_lastchange;
#if IF_LASTCHANGEUPTIME
	/* Crude conversion from uptime to calendar time */
	last_change->tv_sec += boottime_sec();
#endif
	return (0);
}

errno_t
ifnet_get_address_list(ifnet_t interface, ifaddr_t **addresses)
{
	return (addresses == NULL ? EINVAL :
	    ifnet_get_address_list_family(interface, addresses, 0));
}

struct ifnet_addr_list {
	SLIST_ENTRY(ifnet_addr_list)	ifal_le;
	struct ifaddr			*ifal_ifa;
};

errno_t
ifnet_get_address_list_family(ifnet_t interface, ifaddr_t **addresses,
    sa_family_t family)
{
	return (ifnet_get_address_list_family_internal(interface, addresses,
	    family, 0, M_NOWAIT));
}

__private_extern__ errno_t
ifnet_get_address_list_family_internal(ifnet_t interface, ifaddr_t **addresses,
    sa_family_t family, int detached, int how)
{
	SLIST_HEAD(, ifnet_addr_list) ifal_head;
	struct ifnet_addr_list *ifal, *ifal_tmp;
	struct ifnet *ifp;
	int count = 0;
	errno_t err = 0;

	SLIST_INIT(&ifal_head);

	if (addresses == NULL) {
		err = EINVAL;
		goto done;
	}
	*addresses = NULL;

	if (detached) {
		/*
		 * Interface has been detached, so skip the lookup
		 * at ifnet_head and go directly to inner loop.
		 */
		ifp = interface;
		if (ifp == NULL) {
			err = EINVAL;
			goto done;
		}
		goto one;
	}

	ifnet_head_lock_shared();
	TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
		if (interface != NULL && ifp != interface)
			continue;
one:
		ifnet_lock_shared(ifp);
		if (interface == NULL || interface == ifp) {
			struct ifaddr *ifa;
			TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link) {
				IFA_LOCK(ifa);
				if (family != 0 &&
				    ifa->ifa_addr->sa_family != family) {
					IFA_UNLOCK(ifa);
					continue;
				}
				MALLOC(ifal, struct ifnet_addr_list *,
				    sizeof (*ifal), M_TEMP, how);
				if (ifal == NULL) {
					IFA_UNLOCK(ifa);
					ifnet_lock_done(ifp);
					if (!detached)
						ifnet_head_done();
					err = ENOMEM;
					goto done;
				}
				ifal->ifal_ifa = ifa;
				IFA_ADDREF_LOCKED(ifa);
				SLIST_INSERT_HEAD(&ifal_head, ifal, ifal_le);
				++count;
				IFA_UNLOCK(ifa);
			}
		}
		ifnet_lock_done(ifp);
		if (detached)
			break;
	}
	if (!detached)
		ifnet_head_done();

	if (count == 0) {
		err = ENXIO;
		goto done;
	}
	MALLOC(*addresses, ifaddr_t *, sizeof (ifaddr_t) * (count + 1),
	    M_TEMP, how);
	if (*addresses == NULL) {
		err = ENOMEM;
		goto done;
	}
	bzero(*addresses, sizeof (ifaddr_t) * (count + 1));

done:
	SLIST_FOREACH_SAFE(ifal, &ifal_head, ifal_le, ifal_tmp) {
		SLIST_REMOVE(&ifal_head, ifal, ifnet_addr_list, ifal_le);
		if (err == 0)
			(*addresses)[--count] = ifal->ifal_ifa;
		else
			IFA_REMREF(ifal->ifal_ifa);
		FREE(ifal, M_TEMP);
	}

	return (err);
}

void
ifnet_free_address_list(ifaddr_t *addresses)
{
	int i;

	if (addresses == NULL)
		return;

	for (i = 0; addresses[i] != NULL; i++)
		IFA_REMREF(addresses[i]);

	FREE(addresses, M_TEMP);
}

void *
ifnet_lladdr(ifnet_t interface)
{
	struct ifaddr *ifa;
	void *lladdr;

	if (interface == NULL)
		return (NULL);

	/*
	 * if_lladdr points to the permanent link address of
	 * the interface; it never gets deallocated.
	 */
	ifa = interface->if_lladdr;
	IFA_LOCK_SPIN(ifa);
	lladdr = LLADDR(SDL((void *)ifa->ifa_addr));
	IFA_UNLOCK(ifa);

	return (lladdr);
}

errno_t
ifnet_llbroadcast_copy_bytes(ifnet_t interface, void *addr, size_t buffer_len,
    size_t *out_len)
{
	if (interface == NULL || addr == NULL || out_len == NULL)
		return (EINVAL);

	*out_len = interface->if_broadcast.length;

	if (buffer_len < interface->if_broadcast.length)
		return (EMSGSIZE);

	if (interface->if_broadcast.length == 0)
		return (ENXIO);

	if (interface->if_broadcast.length <=
	    sizeof (interface->if_broadcast.u.buffer)) {
		bcopy(interface->if_broadcast.u.buffer, addr,
		    interface->if_broadcast.length);
	} else {
		bcopy(interface->if_broadcast.u.ptr, addr,
		    interface->if_broadcast.length);
	}

	return (0);
}

errno_t
ifnet_lladdr_copy_bytes(ifnet_t interface, void *lladdr, size_t	lladdr_len)
{
	struct sockaddr_dl *sdl;
	struct ifaddr *ifa;

	if (interface == NULL || lladdr == NULL)
		return (EINVAL);

	/*
	 * if_lladdr points to the permanent link address of
	 * the interface; it never gets deallocated.
	 */
	ifa = interface->if_lladdr;
	IFA_LOCK_SPIN(ifa);
	sdl = SDL((void *)ifa->ifa_addr);
	if (lladdr_len != sdl->sdl_alen) {
		bzero(lladdr, lladdr_len);
		IFA_UNLOCK(ifa);
		return (EMSGSIZE);
	}
	bcopy(LLADDR(sdl), lladdr, lladdr_len);
	IFA_UNLOCK(ifa);

	return (0);
}

static errno_t
ifnet_set_lladdr_internal(ifnet_t interface, const void *lladdr,
    size_t lladdr_len, u_char new_type, int apply_type)
{
	struct ifaddr *ifa;
	errno_t	error = 0;

	if (interface == NULL)
		return (EINVAL);

	ifnet_head_lock_shared();
	ifnet_lock_exclusive(interface);
	if (lladdr_len != 0 &&
	    (lladdr_len != interface->if_addrlen || lladdr == 0)) {
		ifnet_lock_done(interface);
		ifnet_head_done();
		return (EINVAL);
	}
	ifa = ifnet_addrs[interface->if_index - 1];
	if (ifa != NULL) {
		struct sockaddr_dl *sdl;

		IFA_LOCK_SPIN(ifa);
		sdl = (struct sockaddr_dl *)(void *)ifa->ifa_addr;
		if (lladdr_len != 0) {
			bcopy(lladdr, LLADDR(sdl), lladdr_len);
		} else {
			bzero(LLADDR(sdl), interface->if_addrlen);
		}
		sdl->sdl_alen = lladdr_len;

		if (apply_type) {
			sdl->sdl_type = new_type;
		}
		IFA_UNLOCK(ifa);
	} else {
		error = ENXIO;
	}
	ifnet_lock_done(interface);
	ifnet_head_done();

	/* Generate a kernel event */
	if (error == 0) {
		dlil_post_msg(interface, KEV_DL_SUBCLASS,
		    KEV_DL_LINK_ADDRESS_CHANGED, NULL, 0);
	}

	return (error);
}

errno_t
ifnet_set_lladdr(ifnet_t interface, const void* lladdr, size_t lladdr_len)
{
	return (ifnet_set_lladdr_internal(interface, lladdr, lladdr_len, 0, 0));
}

errno_t
ifnet_set_lladdr_and_type(ifnet_t interface, const void* lladdr,
    size_t lladdr_len, u_char type)
{
	return (ifnet_set_lladdr_internal(interface, lladdr,
	    lladdr_len, type, 1));
}

errno_t
ifnet_add_multicast(ifnet_t interface, const struct sockaddr *maddr,
    ifmultiaddr_t *ifmap)
{
	if (interface == NULL || maddr == NULL)
		return (EINVAL);

	/* Don't let users screw up protocols' entries. */
	if (maddr->sa_family != AF_UNSPEC && maddr->sa_family != AF_LINK)
		return (EINVAL);

	return (if_addmulti_anon(interface, maddr, ifmap));
}

errno_t
ifnet_remove_multicast(ifmultiaddr_t ifma)
{
	struct sockaddr *maddr;

	if (ifma == NULL)
		return (EINVAL);

	maddr = ifma->ifma_addr;
	/* Don't let users screw up protocols' entries. */
	if (maddr->sa_family != AF_UNSPEC && maddr->sa_family != AF_LINK)
		return (EINVAL);

	return (if_delmulti_anon(ifma->ifma_ifp, maddr));
}

errno_t
ifnet_get_multicast_list(ifnet_t ifp, ifmultiaddr_t **addresses)
{
	int count = 0;
	int cmax = 0;
	struct ifmultiaddr *addr;

	if (ifp == NULL || addresses == NULL)
		return (EINVAL);

	ifnet_lock_shared(ifp);
	LIST_FOREACH(addr, &ifp->if_multiaddrs, ifma_link) {
		cmax++;
	}

	MALLOC(*addresses, ifmultiaddr_t *, sizeof (ifmultiaddr_t) * (cmax + 1),
	    M_TEMP, M_NOWAIT);
	if (*addresses == NULL) {
		ifnet_lock_done(ifp);
		return (ENOMEM);
	}

	LIST_FOREACH(addr, &ifp->if_multiaddrs, ifma_link) {
		if (count + 1 > cmax)
			break;
		(*addresses)[count] = (ifmultiaddr_t)addr;
		ifmaddr_reference((*addresses)[count]);
		count++;
	}
	(*addresses)[cmax] = NULL;
	ifnet_lock_done(ifp);

	return (0);
}

void
ifnet_free_multicast_list(ifmultiaddr_t *addresses)
{
	int i;

	if (addresses == NULL)
		return;

	for (i = 0; addresses[i] != NULL; i++)
		ifmaddr_release(addresses[i]);

	FREE(addresses, M_TEMP);
}

errno_t
ifnet_find_by_name(const char *ifname, ifnet_t *ifpp)
{
	struct ifnet *ifp;
	int	namelen;

	if (ifname == NULL)
		return (EINVAL);

	namelen = strlen(ifname);

	*ifpp = NULL;

	ifnet_head_lock_shared();
	TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
		struct ifaddr *ifa;
		struct sockaddr_dl *ll_addr;

		ifa = ifnet_addrs[ifp->if_index - 1];
		if (ifa == NULL)
			continue;

		IFA_LOCK(ifa);
		ll_addr = (struct sockaddr_dl *)(void *)ifa->ifa_addr;

		if (namelen == ll_addr->sdl_nlen && strncmp(ll_addr->sdl_data,
		    ifname, ll_addr->sdl_nlen) == 0) {
			IFA_UNLOCK(ifa);
			*ifpp = ifp;
			ifnet_reference(*ifpp);
			break;
		}
		IFA_UNLOCK(ifa);
	}
	ifnet_head_done();

	return ((ifp == NULL) ? ENXIO : 0);
}

errno_t
ifnet_list_get(ifnet_family_t family, ifnet_t **list, u_int32_t *count)
{
	return (ifnet_list_get_common(family, FALSE, list, count));
}

__private_extern__ errno_t
ifnet_list_get_all(ifnet_family_t family, ifnet_t **list, u_int32_t *count)
{
	return (ifnet_list_get_common(family, TRUE, list, count));
}

struct ifnet_list {
	SLIST_ENTRY(ifnet_list)	ifl_le;
	struct ifnet		*ifl_ifp;
};

static errno_t
ifnet_list_get_common(ifnet_family_t family, boolean_t get_all, ifnet_t **list,
    u_int32_t *count)
{
#pragma unused(get_all)
	SLIST_HEAD(, ifnet_list) ifl_head;
	struct ifnet_list *ifl, *ifl_tmp;
	struct ifnet *ifp;
	int cnt = 0;
	errno_t err = 0;

	SLIST_INIT(&ifl_head);

	if (list == NULL || count == NULL) {
		err = EINVAL;
		goto done;
	}
	*count = 0;
	*list = NULL;

	ifnet_head_lock_shared();
	TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
		if (family == IFNET_FAMILY_ANY || ifp->if_family == family) {
			MALLOC(ifl, struct ifnet_list *, sizeof (*ifl),
			    M_TEMP, M_NOWAIT);
			if (ifl == NULL) {
				ifnet_head_done();
				err = ENOMEM;
				goto done;
			}
			ifl->ifl_ifp = ifp;
			ifnet_reference(ifp);
			SLIST_INSERT_HEAD(&ifl_head, ifl, ifl_le);
			++cnt;
		}
	}
	ifnet_head_done();

	if (cnt == 0) {
		err = ENXIO;
		goto done;
	}

	MALLOC(*list, ifnet_t *, sizeof (ifnet_t) * (cnt + 1),
	    M_TEMP, M_NOWAIT);
	if (*list == NULL) {
		err = ENOMEM;
		goto done;
	}
	bzero(*list, sizeof (ifnet_t) * (cnt + 1));
	*count = cnt;

done:
	SLIST_FOREACH_SAFE(ifl, &ifl_head, ifl_le, ifl_tmp) {
		SLIST_REMOVE(&ifl_head, ifl, ifnet_list, ifl_le);
		if (err == 0)
			(*list)[--cnt] = ifl->ifl_ifp;
		else
			ifnet_release(ifl->ifl_ifp);
		FREE(ifl, M_TEMP);
	}

	return (err);
}

void
ifnet_list_free(ifnet_t *interfaces)
{
	int i;

	if (interfaces == NULL)
		return;

	for (i = 0; interfaces[i]; i++)
		ifnet_release(interfaces[i]);

	FREE(interfaces, M_TEMP);
}

void
ifnet_transmit_burst_start(ifnet_t ifp, mbuf_t pkt)
{
	uint32_t orig_flags;

	if (ifp == NULL || !(pkt->m_flags & M_PKTHDR))
		return;

	orig_flags = OSBitOrAtomic(IF_MEASURED_BW_INPROGRESS,
	    &ifp->if_bw.flags);
	if (orig_flags & IF_MEASURED_BW_INPROGRESS) {
		/* There is already a measurement in progress; skip this one */
		return;
	}

	ifp->if_bw.start_seq = pkt->m_pkthdr.pf_mtag.pftag_pktseq;
	ifp->if_bw.start_ts = mach_absolute_time();
}

void
ifnet_transmit_burst_end(ifnet_t ifp, mbuf_t pkt)
{
	uint64_t oseq, ots, bytes, ts, t;
	uint32_t flags;

	if ( ifp == NULL || !(pkt->m_flags & M_PKTHDR))
		return;

	flags = OSBitOrAtomic(IF_MEASURED_BW_CALCULATION, &ifp->if_bw.flags);

	/* If a calculation is already in progress, just return */
	if (flags & IF_MEASURED_BW_CALCULATION)
		return;

	/* Check if a measurement was started at all */
	if (!(flags & IF_MEASURED_BW_INPROGRESS)) {
		/*
		 * It is an error to call burst_end before burst_start.
		 * Reset the calculation flag and return.
		 */
		goto done;
	}

	oseq = pkt->m_pkthdr.pf_mtag.pftag_pktseq;
	ots = mach_absolute_time();

	if (ifp->if_bw.start_seq > 0 && oseq > ifp->if_bw.start_seq) {
		ts = ots - ifp->if_bw.start_ts;
		if (ts > 0 ) {
			absolutetime_to_nanoseconds(ts, &t);
			bytes = oseq - ifp->if_bw.start_seq;
			ifp->if_bw.bytes = bytes;
			ifp->if_bw.ts = ts;

			if (t > 0) {
				uint64_t bw = 0;

				/* Compute bandwidth as bytes/ms */
				bw = (bytes * NSEC_PER_MSEC) / t;
				if (bw > 0) {
					if (ifp->if_bw.bw > 0) {
						u_int32_t shft;

						shft = if_bw_smoothing_val;
						/* Compute EWMA of bw */
						ifp->if_bw.bw = (bw +
						    ((ifp->if_bw.bw << shft) -
						    ifp->if_bw.bw)) >> shft;
					} else {
						ifp->if_bw.bw = bw;
					}
				}
			}
			ifp->if_bw.last_seq = oseq;
			ifp->if_bw.last_ts = ots;
		}
	}

done:
	flags = ~(IF_MEASURED_BW_INPROGRESS | IF_MEASURED_BW_CALCULATION);
	OSBitAndAtomic(flags, &ifp->if_bw.flags);
}

/****************************************************************************/
/* ifaddr_t accessors							    */
/****************************************************************************/

errno_t
ifaddr_reference(ifaddr_t ifa)
{
	if (ifa == NULL)
		return (EINVAL);

	IFA_ADDREF(ifa);
	return (0);
}

errno_t
ifaddr_release(ifaddr_t ifa)
{
	if (ifa == NULL)
		return (EINVAL);

	IFA_REMREF(ifa);
	return (0);
}

sa_family_t
ifaddr_address_family(ifaddr_t ifa)
{
	sa_family_t family = 0;

	if (ifa != NULL) {
		IFA_LOCK_SPIN(ifa);
		if (ifa->ifa_addr != NULL)
			family = ifa->ifa_addr->sa_family;
		IFA_UNLOCK(ifa);
	}
	return (family);
}

errno_t
ifaddr_address(ifaddr_t ifa, struct sockaddr *out_addr, u_int32_t addr_size)
{
	u_int32_t copylen;

	if (ifa == NULL || out_addr == NULL)
		return (EINVAL);

	IFA_LOCK_SPIN(ifa);
	if (ifa->ifa_addr == NULL) {
		IFA_UNLOCK(ifa);
		return (ENOTSUP);
	}

	copylen = (addr_size >= ifa->ifa_addr->sa_len) ?
	    ifa->ifa_addr->sa_len : addr_size;
	bcopy(ifa->ifa_addr, out_addr, copylen);

	if (ifa->ifa_addr->sa_len > addr_size) {
		IFA_UNLOCK(ifa);
		return (EMSGSIZE);
	}

	IFA_UNLOCK(ifa);
	return (0);
}

errno_t
ifaddr_dstaddress(ifaddr_t ifa, struct sockaddr *out_addr, u_int32_t addr_size)
{
	u_int32_t copylen;

	if (ifa == NULL || out_addr == NULL)
		return (EINVAL);

	IFA_LOCK_SPIN(ifa);
	if (ifa->ifa_dstaddr == NULL) {
		IFA_UNLOCK(ifa);
		return (ENOTSUP);
	}

	copylen = (addr_size >= ifa->ifa_dstaddr->sa_len) ?
	    ifa->ifa_dstaddr->sa_len : addr_size;
	bcopy(ifa->ifa_dstaddr, out_addr, copylen);

	if (ifa->ifa_dstaddr->sa_len > addr_size) {
		IFA_UNLOCK(ifa);
		return (EMSGSIZE);
	}

	IFA_UNLOCK(ifa);
	return (0);
}

errno_t
ifaddr_netmask(ifaddr_t ifa, struct sockaddr *out_addr, u_int32_t addr_size)
{
	u_int32_t copylen;

	if (ifa == NULL || out_addr == NULL)
		return (EINVAL);

	IFA_LOCK_SPIN(ifa);
	if (ifa->ifa_netmask == NULL) {
		IFA_UNLOCK(ifa);
		return (ENOTSUP);
	}

	copylen = addr_size >= ifa->ifa_netmask->sa_len ?
	    ifa->ifa_netmask->sa_len : addr_size;
	bcopy(ifa->ifa_netmask, out_addr, copylen);

	if (ifa->ifa_netmask->sa_len > addr_size) {
		IFA_UNLOCK(ifa);
		return (EMSGSIZE);
	}

	IFA_UNLOCK(ifa);
	return (0);
}

ifnet_t
ifaddr_ifnet(ifaddr_t ifa)
{
	struct ifnet *ifp;

	if (ifa == NULL)
		return (NULL);

	/* ifa_ifp is set once at creation time; it is never changed */
	ifp = ifa->ifa_ifp;

	return (ifp);
}

ifaddr_t
ifaddr_withaddr(const struct sockaddr *address)
{
	if (address == NULL)
		return (NULL);

	return (ifa_ifwithaddr(address));
}

ifaddr_t
ifaddr_withdstaddr(const struct sockaddr *address)
{
	if (address == NULL)
		return (NULL);

	return (ifa_ifwithdstaddr(address));
}

ifaddr_t
ifaddr_withnet(const struct sockaddr *net)
{
	if (net == NULL)
		return (NULL);

	return (ifa_ifwithnet(net));
}

ifaddr_t
ifaddr_withroute(int flags, const struct sockaddr *destination,
    const struct sockaddr *gateway)
{
	if (destination == NULL || gateway == NULL)
		return (NULL);

	return (ifa_ifwithroute(flags, destination, gateway));
}

ifaddr_t
ifaddr_findbestforaddr(const struct sockaddr *addr, ifnet_t interface)
{
	if (addr == NULL || interface == NULL)
		return (NULL);

	return (ifaof_ifpforaddr(addr, interface));
}

errno_t
ifmaddr_reference(ifmultiaddr_t ifmaddr)
{
	if (ifmaddr == NULL)
		return (EINVAL);

	IFMA_ADDREF(ifmaddr);
	return (0);
}

errno_t
ifmaddr_release(ifmultiaddr_t ifmaddr)
{
	if (ifmaddr == NULL)
		return (EINVAL);

	IFMA_REMREF(ifmaddr);
	return (0);
}

errno_t
ifmaddr_address(ifmultiaddr_t ifma, struct sockaddr *out_addr,
    u_int32_t addr_size)
{
	u_int32_t copylen;

	if (ifma == NULL || out_addr == NULL)
		return (EINVAL);

	IFMA_LOCK(ifma);
	if (ifma->ifma_addr == NULL) {
		IFMA_UNLOCK(ifma);
		return (ENOTSUP);
	}

	copylen = (addr_size >= ifma->ifma_addr->sa_len ?
	    ifma->ifma_addr->sa_len : addr_size);
	bcopy(ifma->ifma_addr, out_addr, copylen);

	if (ifma->ifma_addr->sa_len > addr_size) {
		IFMA_UNLOCK(ifma);
		return (EMSGSIZE);
	}
	IFMA_UNLOCK(ifma);
	return (0);
}

errno_t
ifmaddr_lladdress(ifmultiaddr_t ifma, struct sockaddr *out_addr,
    u_int32_t addr_size)
{
	struct ifmultiaddr *ifma_ll;

	if (ifma == NULL || out_addr == NULL)
		return (EINVAL);
	if ((ifma_ll = ifma->ifma_ll) == NULL)
		return (ENOTSUP);

	return (ifmaddr_address(ifma_ll, out_addr, addr_size));
}

ifnet_t
ifmaddr_ifnet(ifmultiaddr_t ifma)
{
	return ((ifma == NULL) ? NULL : ifma->ifma_ifp);
}

/******************************************************************************/
/* interface cloner                                                           */
/******************************************************************************/

errno_t
ifnet_clone_attach(struct ifnet_clone_params *cloner_params,
    if_clone_t *ifcloner)
{
	errno_t error = 0;
	struct if_clone *ifc = NULL;
	size_t namelen;

	if (cloner_params == NULL || ifcloner == NULL ||
	    cloner_params->ifc_name == NULL ||
	    cloner_params->ifc_create == NULL ||
	    cloner_params->ifc_destroy == NULL ||
	    (namelen = strlen(cloner_params->ifc_name)) >= IFNAMSIZ) {
		error = EINVAL;
		goto fail;
	}

	if (if_clone_lookup(cloner_params->ifc_name, NULL) != NULL) {
		printf("%s: already a cloner for %s\n", __func__,
		    cloner_params->ifc_name);
		error = EEXIST;
		goto fail;
	}

	/* Make room for name string */
	ifc = _MALLOC(sizeof (struct if_clone) + IFNAMSIZ + 1, M_CLONE,
	    M_WAITOK | M_ZERO);
	if (ifc == NULL) {
		printf("%s: _MALLOC failed\n", __func__);
		error = ENOBUFS;
		goto fail;
	}
	strlcpy((char *)(ifc + 1), cloner_params->ifc_name, IFNAMSIZ + 1);
	ifc->ifc_name = (char *)(ifc + 1);
	ifc->ifc_namelen = namelen;
	ifc->ifc_maxunit = IF_MAXUNIT;
	ifc->ifc_create = cloner_params->ifc_create;
	ifc->ifc_destroy = cloner_params->ifc_destroy;

	error = if_clone_attach(ifc);
	if (error != 0) {
		printf("%s: if_clone_attach failed %d\n", __func__, error);
		goto fail;
	}
	*ifcloner = ifc;

	return (0);
fail:
	if (ifc != NULL)
		FREE(ifc, M_CLONE);
	return (error);
}

errno_t
ifnet_clone_detach(if_clone_t ifcloner)
{
	errno_t error = 0;
	struct if_clone *ifc = ifcloner;

	if (ifc == NULL || ifc->ifc_name == NULL)
		return (EINVAL);

	if ((if_clone_lookup(ifc->ifc_name, NULL)) == NULL) {
		printf("%s: no cloner for %s\n", __func__, ifc->ifc_name);
		error = EINVAL;
		goto fail;
	}

	if_clone_detach(ifc);

	FREE(ifc, M_CLONE);

fail:
	return (error);
}

/******************************************************************************/
/* misc                                                                       */
/******************************************************************************/

extern void udp_get_ports_used(unsigned int ifindex, uint8_t *bitfield);
extern void tcp_get_ports_used(unsigned int ifindex, uint8_t *bitfield);

errno_t
ifnet_get_local_ports(ifnet_t ifp, uint8_t *bitfield)
{
	if (bitfield == NULL)
		return (EINVAL);

	bzero(bitfield, 8192);

	udp_get_ports_used(ifp ? ifp->if_index : 0, bitfield);
	tcp_get_ports_used(ifp ? ifp->if_index : 0, bitfield);

	return (0);
}

errno_t
ifnet_notice_node_presence(ifnet_t ifp, struct sockaddr* sa, int32_t rssi,
    int lqm, int npm, u_int8_t srvinfo[48])
{
	if (ifp == NULL || sa == NULL || srvinfo == NULL)
		return(EINVAL);
	if (sa->sa_len > sizeof(struct sockaddr_storage))
		return(EINVAL);
	if (sa->sa_family != AF_LINK && sa->sa_family != AF_INET6)
		return(EINVAL);
	
	dlil_node_present(ifp, sa, rssi, lqm, npm, srvinfo);
	return (0);
}

errno_t
ifnet_notice_node_absence(ifnet_t ifp, struct sockaddr* sa)
{
	if (ifp == NULL || sa == NULL)
		return(EINVAL);
	if (sa->sa_len > sizeof(struct sockaddr_storage))
		return(EINVAL);
	if (sa->sa_family != AF_LINK && sa->sa_family != AF_INET6)
		return(EINVAL);
	
	dlil_node_absent(ifp, sa);
	return (0);
}

errno_t
ifnet_notice_master_elected(ifnet_t ifp)
{
	if (ifp == NULL)
		return(EINVAL);
	
	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_MASTER_ELECTED, NULL, 0);
	return (0);
}
