 /*
 * Copyright (C) 2019 Allwinnertech Co.Ltd
 * Authors: zhengwanyu
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#ifndef _SUNXI_CONFIG_H_
#define _SUNXI_CONFIG_H_

/* define DE Version */
#if defined(CONFIG_ARCH_SUN50IW1)
#define LOWLEVEL_SUN50IW1

#elif defined(CONFIG_ARCH_SUN8IW11) \
	|| defined(CONFIG_ARCH_SUN8IW12) \
	|| defined(CONFIG_ARCH_SUN8IW15) \
	|| defined(CONFIG_ARCH_SUN8IW6) \
	|| defined(CONFIG_ARCH_SUN8IW7) \
	|| defined(CONFIG_ARCH_SUN50IW8)
#define LOWLEVEL_V2X

#elif defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN50IW6)
#define LOWLEVEL_V3X

#elif defined(CONFIG_ARCH_SUN50IW5T) \
	|| defined(CONFIG_ARCH_SUN50IW9)
#define LOWLEVEL_V33X

#else
/* "Compile ERROR!!!" */
#endif

#endif
