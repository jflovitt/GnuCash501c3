/* gnc-nonprofit.h -- book-level nonprofit / 501(c)(3) helpers
 *
 * Copyright (C) 2026 GnuCash501c3 contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef GNC_NONPROFIT_H
#define GNC_NONPROFIT_H

#include "qof.h"
#include "gnc-numeric.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Book KVP path for nonprofit settings. */
#define GNC_NONPROFIT_PATH "nonprofit"

/** Month of fiscal year end (1-12). Default 12 (calendar year). */
#define GNC_NONPROFIT_FY_END_MONTH "fy-end-month"
/** Day of fiscal year end. Default 31. */
#define GNC_NONPROFIT_FY_END_DAY "fy-end-day"
/** Schedule B–style contribution threshold (USD major units as double). Default 5000. */
#define GNC_NONPROFIT_CONTRIB_THRESHOLD "contrib-threshold"

gint gnc_nonprofit_get_fy_end_month (QofBook *book);
gint gnc_nonprofit_get_fy_end_day (QofBook *book);
void gnc_nonprofit_set_fy_end (QofBook *book, gint month, gint day);

/** Contribution reporting threshold in book currency major units (e.g. 5000.0). */
double gnc_nonprofit_get_contrib_threshold (QofBook *book);
void gnc_nonprofit_set_contrib_threshold (QofBook *book, double amount);

#ifdef __cplusplus
}
#endif

#endif /* GNC_NONPROFIT_H */
