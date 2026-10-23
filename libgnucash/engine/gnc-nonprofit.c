/* gnc-nonprofit.c -- book-level nonprofit / 501(c)(3) helpers */

#include <config.h>
#include <glib.h>
#include "gnc-nonprofit.h"
#include "qofbookslots.h"
#include "qofinstance-p.h"

#define DEFAULT_THRESHOLD 5000.0

gint
gnc_nonprofit_get_fy_end_month (QofBook *book)
{
    GValue v = G_VALUE_INIT;
    gint month = 12;
    g_return_val_if_fail (book, 12);
    g_value_init (&v, G_TYPE_INT64);
    qof_instance_get_kvp (QOF_INSTANCE (book), &v, 2, GNC_NONPROFIT_PATH, GNC_NONPROFIT_FY_END_MONTH);
    if (G_VALUE_HOLDS_INT64 (&v))
    {
        gint64 m = g_value_get_int64 (&v);
        if (m >= 1 && m <= 12)
            month = (gint) m;
    }
    g_value_unset (&v);
    return month;
}

gint
gnc_nonprofit_get_fy_end_day (QofBook *book)
{
    GValue v = G_VALUE_INIT;
    gint day = 31;
    g_return_val_if_fail (book, 31);
    g_value_init (&v, G_TYPE_INT64);
    qof_instance_get_kvp (QOF_INSTANCE (book), &v, 2, GNC_NONPROFIT_PATH, GNC_NONPROFIT_FY_END_DAY);
    if (G_VALUE_HOLDS_INT64 (&v))
    {
        gint64 d = g_value_get_int64 (&v);
        if (d >= 1 && d <= 31)
            day = (gint) d;
    }
    g_value_unset (&v);
    return day;
}

void
gnc_nonprofit_set_fy_end (QofBook *book, gint month, gint day)
{
    GValue v = G_VALUE_INIT;
    g_return_if_fail (book);
    g_return_if_fail (month >= 1 && month <= 12);
    g_return_if_fail (day >= 1 && day <= 31);
    qof_book_begin_edit (book);
    g_value_init (&v, G_TYPE_INT64);
    g_value_set_int64 (&v, month);
    qof_instance_set_kvp (QOF_INSTANCE (book), &v, 2, GNC_NONPROFIT_PATH, GNC_NONPROFIT_FY_END_MONTH);
    g_value_unset (&v);
    g_value_init (&v, G_TYPE_INT64);
    g_value_set_int64 (&v, day);
    qof_instance_set_kvp (QOF_INSTANCE (book), &v, 2, GNC_NONPROFIT_PATH, GNC_NONPROFIT_FY_END_DAY);
    g_value_unset (&v);
    qof_instance_set_dirty (QOF_INSTANCE (book));
    qof_book_commit_edit (book);
}

double
gnc_nonprofit_get_contrib_threshold (QofBook *book)
{
    GValue v = G_VALUE_INIT;
    double thr = DEFAULT_THRESHOLD;
    g_return_val_if_fail (book, DEFAULT_THRESHOLD);
    g_value_init (&v, G_TYPE_DOUBLE);
    qof_instance_get_kvp (QOF_INSTANCE (book), &v, 2, GNC_NONPROFIT_PATH, GNC_NONPROFIT_CONTRIB_THRESHOLD);
    if (G_VALUE_HOLDS_DOUBLE (&v))
    {
        double d = g_value_get_double (&v);
        if (d > 0.0)
            thr = d;
    }
    g_value_unset (&v);
    return thr;
}

void
gnc_nonprofit_set_contrib_threshold (QofBook *book, double amount)
{
    GValue v = G_VALUE_INIT;
    g_return_if_fail (book);
    g_return_if_fail (amount > 0.0);
    qof_book_begin_edit (book);
    g_value_init (&v, G_TYPE_DOUBLE);
    g_value_set_double (&v, amount);
    qof_instance_set_kvp (QOF_INSTANCE (book), &v, 2, GNC_NONPROFIT_PATH, GNC_NONPROFIT_CONTRIB_THRESHOLD);
    g_value_unset (&v);
    qof_instance_set_dirty (QOF_INSTANCE (book));
    qof_book_commit_edit (book);
}
