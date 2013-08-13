/*
 * Copyright 2013, winocm. <rms@velocitylimitless.org>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 *   If you are going to use this software in any form that does not involve
 *   releasing the source to this project or improving it, let me know beforehand.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * SoC system service dispatch table structure thing that sounds
 * really cool and awesome yet is really and stupidly simple.
 *
 * Whatever.
 */

#include <mach/mach_types.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

SocDeviceDispatch    gPESocDispatch;

void PE_init_SocSupport(void)
{
    PE_early_puts("PE_init_SocSupport: initializing SoC dispatch\n");
#ifdef BOARD_CONFIG_ARMPBA8
    /*
     * Do the soc dispatch stuff for our platform.
     */
    PE_early_puts("PE_init_SocSupport: Initializing for ARM RealView PB-A8\n");
    PE_init_SocSupport_realview();
#elif defined(BOARD_CONFIG_SUN4I)
    PE_init_SocSupport_sun4i();
#elif defined(BOARD_CONFIG_OMAP3530)
    PE_init_SocSupport_omap3();
#else
#error You need to fill out PE_init_SocSupport for your board!
#endif
    return;
}