/********************************************************************\
 * gncDonor.c -- the Core Donor Interface                     *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/*
 * Copyright (C) 2001,2002 Derek Atkins
 * Copyright (C) 2003 Linas Vepstas <linas@linas.org>
 * Author: Derek Atkins <warlord@MIT.EDU>
 */

#include <config.h>

#include <glib.h>
#include <string.h>
#include <qofinstance-p.h>

#include "Account.h"
#include "gnc-commodity.h"
#include "gncAddressP.h"
#include "gncDonor.h"
#include "gncDonorP.h"
#include "gnc-lot.h"
#include "gncOwner.h"

static gint donor_qof_event_handler_id = 0;
static void donor_handle_qof_events (QofInstance *entity, QofEventId event_type,
                                    gpointer user_data, gpointer event_data);

struct _gncDonor
{
    QofInstance     inst;
    const char *    id;
    const char *    username;
    GncAddress *    addr;
    gnc_commodity * currency;
    gboolean        active;
    gnc_numeric *   balance;

    const char *    language;
    const char *    acl;
    gnc_numeric     workday;
    gnc_numeric     rate;

    Account *        ccard_acc;
};

struct _gncDonorClass
{
    QofInstanceClass parent_class;
};

static QofLogModule log_module = GNC_MOD_BUSINESS;

#define _GNC_MOD_NAME        GNC_ID_DONOR

static inline void mark_donor (GncDonor *donor);
void mark_donor (GncDonor *donor)
{
    qof_instance_set_dirty(&donor->inst);
    qof_event_gen (&donor->inst, QOF_EVENT_MODIFY, NULL);
}

/* ============================================================== */

enum
{
    PROP_0,
    PROP_USERNAME,		/* Table */
    PROP_ID,			/* Table */
    PROP_LANGUAGE,		/* Table */
    PROP_ACL,			/* Table */
    PROP_ACTIVE,		/* Table */
    PROP_CURRENCY,		/* Table */
    PROP_CCARD,			/* Table */
    PROP_WORKDAY,		/* Table (numeric) */
    PROP_RATE,			/* Table (numeric) */
    PROP_ADDRESS,		/* Table, 8 fields */
    PROP_PDF_DIRNAME,		/* KVP */
    PROP_LAST_POSTED,		/* KVP */
    PROP_PAYMENT_LAST_ACCT,	/* KVP */
};

/* GObject Initialization */
G_DEFINE_TYPE(GncDonor, gnc_donor, QOF_TYPE_INSTANCE)

static void
gnc_donor_init(GncDonor* emp)
{
}

static void
gnc_donor_dispose(GObject *empp)
{
    G_OBJECT_CLASS(gnc_donor_parent_class)->dispose(empp);
}

static void
gnc_donor_finalize(GObject* empp)
{
    G_OBJECT_CLASS(gnc_donor_parent_class)->finalize(empp);
}

/* Note that g_value_set_object() refs the object, as does
 * g_object_get(). But g_object_get() only unrefs once when it disgorges
 * the object, leaving an unbalanced ref, which leaks. So instead of
 * using g_value_set_object(), use g_value_take_object() which doesn't
 * ref the object when used in get_property().
 */
static void
gnc_donor_get_property (GObject         *object,
                           guint            prop_id,
                           GValue          *value,
                           GParamSpec      *pspec)
{
    GncDonor *emp;
    g_return_if_fail(GNC_IS_DONOR(object));
    emp = GNC_DONOR(object);
    switch (prop_id)
    {
    case PROP_USERNAME:
        g_value_set_string(value, emp->username);
        break;
    case PROP_ID:
        g_value_set_string(value, emp->id);
        break;
    case PROP_ACTIVE:
        g_value_set_boolean(value, emp->active);
        break;
    case PROP_LANGUAGE:
        g_value_set_string(value, emp->language);
        break;
    case PROP_CURRENCY:
        g_value_take_object(value, emp->currency);
        break;
    case PROP_ACL:
        g_value_set_string(value, emp->acl);
        break;
    case PROP_ADDRESS:
        g_value_take_object(value, emp->addr);
        break;
    case PROP_WORKDAY:
        g_value_set_boxed(value, &emp->workday);
        break;
    case PROP_RATE:
        g_value_set_boxed(value, &emp->rate);
        break;
    case PROP_CCARD:
        g_value_take_object(value, emp->ccard_acc);
        break;
    case PROP_PDF_DIRNAME:
        qof_instance_get_kvp (QOF_INSTANCE (emp), value, 1, OWNER_EXPORT_PDF_DIRNAME);
        break;
    case PROP_LAST_POSTED:
        qof_instance_get_kvp (QOF_INSTANCE (emp), value, 1, LAST_POSTED_TO_ACCT);
        break;
    case PROP_PAYMENT_LAST_ACCT:
        qof_instance_get_kvp (QOF_INSTANCE (emp), value, 2, GNC_PAYMENT, GNC_LAST_ACCOUNT);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gnc_donor_set_property (GObject         *object,
                           guint            prop_id,
                           const GValue          *value,
                           GParamSpec      *pspec)
{
    GncDonor *emp;
    g_return_if_fail(GNC_IS_DONOR(object));
    emp = GNC_DONOR(object);
    g_assert (qof_instance_get_editlevel(emp));
    switch (prop_id)
    {
    case PROP_USERNAME:
        gncDonorSetUsername(emp, g_value_get_string(value));
        break;
    case PROP_ID:
        gncDonorSetID(emp, g_value_get_string(value));
        break;
    case PROP_ACTIVE:
        gncDonorSetActive(emp, g_value_get_boolean(value));
        break;
    case PROP_LANGUAGE:
        gncDonorSetLanguage(emp, g_value_get_string(value));
        break;
    case PROP_CURRENCY:
        gncDonorSetCurrency(emp, g_value_get_object(value));
        break;
    case PROP_ACL:
        gncDonorSetAcl(emp, g_value_get_string(value));
        break;
    case PROP_ADDRESS:
        qofDonorSetAddr(emp, g_value_get_object(value));
        break;
    case PROP_WORKDAY:
        gncDonorSetWorkday(emp, *(gnc_numeric*)g_value_get_boxed(value));
        break;
    case PROP_RATE:
        gncDonorSetRate(emp, *(gnc_numeric*)g_value_get_boxed(value));
        break;
    case PROP_CCARD:
        gncDonorSetCCard(emp, g_value_get_object(value));
        break;
    case PROP_PDF_DIRNAME:
        qof_instance_set_kvp (QOF_INSTANCE (emp), value, 1, OWNER_EXPORT_PDF_DIRNAME);
        break;
    case PROP_LAST_POSTED:
        qof_instance_set_kvp (QOF_INSTANCE (emp), value, 1, LAST_POSTED_TO_ACCT);
        break;
    case PROP_PAYMENT_LAST_ACCT:
        qof_instance_set_kvp (QOF_INSTANCE (emp), value, 2, GNC_PAYMENT, GNC_LAST_ACCOUNT);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/** Does this object refer to a specific object */
static gboolean
impl_refers_to_object(const QofInstance* inst, const QofInstance* ref)
{
    GncDonor* emp;

    g_return_val_if_fail(inst != NULL, FALSE);
    g_return_val_if_fail(GNC_IS_DONOR(inst), FALSE);

    emp = GNC_DONOR(inst);

    if (GNC_IS_COMMODITY(ref))
    {
        return (emp->currency == GNC_COMMODITY(ref));
    }
    else if (GNC_IS_ACCOUNT(ref))
    {
        return (emp->ccard_acc == GNC_ACCOUNT(ref));
    }

    return FALSE;
}

/** Returns a list of my type of object which refers to an object.  For example, when called as
        qof_instance_get_typed_referring_object_list(taxtable, account);
    it will return the list of taxtables which refer to a specific account.  The result should be the
    same regardless of which taxtable object is used.  The list must be freed by the caller but the
    objects on the list must not.
 */
static GList*
impl_get_typed_referring_object_list(const QofInstance* inst, const QofInstance* ref)
{
    if (!GNC_IS_COMMODITY(ref) && !GNC_IS_ACCOUNT(ref))
    {
        return NULL;
    }

    return qof_instance_get_referring_object_list_from_collection(qof_instance_get_collection(inst), ref);
}

static void
gnc_donor_class_init (GncDonorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    QofInstanceClass* qof_class = QOF_INSTANCE_CLASS(klass);

    gobject_class->dispose = gnc_donor_dispose;
    gobject_class->finalize = gnc_donor_finalize;
    gobject_class->set_property = gnc_donor_set_property;
    gobject_class->get_property = gnc_donor_get_property;

    qof_class->get_display_name = NULL;
    qof_class->refers_to_object = impl_refers_to_object;
    qof_class->get_typed_referring_object_list = impl_get_typed_referring_object_list;

    g_object_class_install_property
    (gobject_class,
     PROP_USERNAME,
     g_param_spec_string ("username",
                          "Donor Name",
                          "The donor name is an arbitrary string "
                          "assigned by the user which provides the donor "
                          "name.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_ID,
     g_param_spec_string ("id",
                          "Donor ID",
                          "The donor ID is an arbitrary string "
                          "assigned by the user which provides the donor "
                          "ID.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_ACTIVE,
     g_param_spec_boolean ("active",
                           "Active",
                           "TRUE if the donor is active.  FALSE if inactive.",
                           FALSE,
                           G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_LANGUAGE,
     g_param_spec_string ("language",
                          "Donor Language",
                          "The language is an arbitrary string "
                          "assigned by the user which provides the language spoken "
                          " by the donor.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_CURRENCY,
     g_param_spec_object ("currency",
                          "Currency",
                          "The currency property denotes the currency used by this donor.",
                          GNC_TYPE_COMMODITY,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_ACL,
     g_param_spec_string ("acl",
                          "Donor ACL",
                          "The acl is an arbitrary string "
                          "assigned by the user which provides ??? "
                          " for the donor.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_ADDRESS,
     g_param_spec_object ("address",
                          "Address",
                          "The address property contains the address information for this donor.",
                          GNC_TYPE_ADDRESS,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_WORKDAY,
     g_param_spec_boxed("workday",
                        "Workday rate",
                        "The daily rate for this donor.",
                        GNC_TYPE_NUMERIC,
                        G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_RATE,
     g_param_spec_boxed("rate",
                        "Hourly rate",
                        "The hourly rate for this donor.",
                        GNC_TYPE_NUMERIC,
                        G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_CCARD,
     g_param_spec_object ("credit-card-account",
                          "Credit card account",
                          "The credit card account for this donor.",
                          GNC_TYPE_ACCOUNT,
                          G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_PDF_DIRNAME,
     g_param_spec_string ("export-pdf-dir",
                          "Export PDF Directory Name",
                          "A subdirectory for exporting PDF reports which is "
			  "appended to the target directory when writing them "
			  "out. It is retrieved from preferences and stored on "
			  "each 'Owner' object which prints items after "
			  "printing.",
                          NULL,
                          G_PARAM_READWRITE));

    g_object_class_install_property(
       gobject_class,
       PROP_LAST_POSTED,
       g_param_spec_boxed("invoice-last-posted-account",
			  "Invoice Last Posted Account",
			  "The last account to which an invoice belonging to "
			  "this owner was posted.",
			  GNC_TYPE_GUID,
			  G_PARAM_READWRITE));

    g_object_class_install_property(
       gobject_class,
       PROP_PAYMENT_LAST_ACCT,
       g_param_spec_boxed("payment-last-account",
			  "Payment Last Account",
			  "The last account to which an payment belonging to "
			  "this owner was posted.",
			  GNC_TYPE_GUID,
			  G_PARAM_READWRITE));
}

/* Create/Destroy Functions */
GncDonor *gncDonorCreate (QofBook *book)
{
    GncDonor *donor;

    if (!book) return NULL;

    donor = g_object_new (GNC_TYPE_DONOR, NULL);
    qof_instance_init_data (&donor->inst, _GNC_MOD_NAME, book);

    donor->id = CACHE_INSERT ("");
    donor->username = CACHE_INSERT ("");
    donor->language = CACHE_INSERT ("");
    donor->acl = CACHE_INSERT ("");
    donor->addr = gncAddressCreate (book, &donor->inst);
    donor->workday = gnc_numeric_zero();
    donor->rate = gnc_numeric_zero();
    donor->active = TRUE;
    donor->balance = NULL;

    if (donor_qof_event_handler_id == 0)
        donor_qof_event_handler_id = qof_event_register_handler (donor_handle_qof_events, NULL);

    qof_event_gen (&donor->inst, QOF_EVENT_CREATE, NULL);

    return donor;
}

void gncDonorDestroy (GncDonor *donor)
{
    if (!donor) return;
    qof_instance_set_destroying(donor, TRUE);
    gncDonorCommitEdit(donor);
}

static void gncDonorFree (GncDonor *donor)
{
    if (!donor) return;

    qof_event_gen (&donor->inst, QOF_EVENT_DESTROY, NULL);

    CACHE_REMOVE (donor->id);
    CACHE_REMOVE (donor->username);
    CACHE_REMOVE (donor->language);
    CACHE_REMOVE (donor->acl);
    gncAddressBeginEdit (donor->addr);
    gncAddressDestroy (donor->addr);
    g_free (donor->balance);

    /* qof_instance_release (&donor->inst); */
    g_object_unref (donor);
}

/* ============================================================== */
/* Set Functions */

#define SET_STR(obj, member, str) { \
        if (!g_strcmp0 (member, str)) return; \
        gncDonorBeginEdit (obj); \
        CACHE_REPLACE (member, str); \
        }

void gncDonorSetID (GncDonor *donor, const char *id)
{
    if (!donor) return;
    if (!id) return;
    SET_STR(donor, donor->id, id);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetUsername (GncDonor *donor, const char *username)
{
    if (!donor) return;
    if (!username) return;
    SET_STR(donor, donor->username, username);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

/* Donors don't have a name property defined, but
 * in order to get a consistent interface with other owner types,
 * this function fakes one by setting the name property of
 * the donor's address.
 */
void gncDonorSetName (GncDonor *donor, const char *name)
{
    if (!donor) return;
    if (!name) return;
    gncAddressSetName (gncDonorGetAddr (donor), name);
}

void gncDonorSetLanguage (GncDonor *donor, const char *language)
{
    if (!donor) return;
    if (!language) return;
    SET_STR(donor, donor->language, language);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetAcl (GncDonor *donor, const char *acl)
{
    if (!donor) return;
    if (!acl) return;
    SET_STR(donor, donor->acl, acl);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetWorkday (GncDonor *donor, gnc_numeric workday)
{
    if (!donor) return;
    if (gnc_numeric_equal (workday, donor->workday)) return;
    gncDonorBeginEdit (donor);
    donor->workday = workday;
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetRate (GncDonor *donor, gnc_numeric rate)
{
    if (!donor) return;
    if (gnc_numeric_equal (rate, donor->rate)) return;
    gncDonorBeginEdit (donor);
    donor->rate = rate;
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetCurrency (GncDonor *donor, gnc_commodity *currency)
{
    if (!donor || !currency) return;
    if (donor->currency &&
            gnc_commodity_equal (donor->currency, currency))
        return;
    gncDonorBeginEdit (donor);
    donor->currency = currency;
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetActive (GncDonor *donor, gboolean active)
{
    if (!donor) return;
    if (active == donor->active) return;
    gncDonorBeginEdit (donor);
    donor->active = active;
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void gncDonorSetCCard (GncDonor *donor, Account* ccard_acc)
{
    if (!donor) return;
    if (ccard_acc == donor->ccard_acc) return;
    gncDonorBeginEdit (donor);
    donor->ccard_acc = ccard_acc;
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

void
qofDonorSetAddr (GncDonor *donor, QofInstance *addr_ent)
{
    GncAddress *addr;

    if (!donor || !addr_ent)
    {
        return;
    }
    addr = (GncAddress*)addr_ent;
    if (addr == donor->addr)
    {
        return;
    }
    if (donor->addr != NULL)
    {
        gncAddressBeginEdit(donor->addr);
        gncAddressDestroy(donor->addr);
    }
    gncDonorBeginEdit(donor);
    donor->addr = addr;
    mark_donor (donor);
    gncDonorCommitEdit(donor);
}

/* ============================================================== */
/* Get Functions */
const char * gncDonorGetID (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->id;
}

const char * gncDonorGetUsername (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->username;
}

/* Donors don't have a name property defined, but
 * in order to get a consistent interface with other owner types,
 * this function fakes one by returning the name property of
 * the donor's address.
 */
const char * gncDonorGetName (const GncDonor *donor)
{
    if (!donor) return NULL;
    return gncAddressGetName ( gncDonorGetAddr (donor));
}

GncAddress * gncDonorGetAddr (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->addr;
}

const char * gncDonorGetLanguage (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->language;
}

const char * gncDonorGetAcl (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->acl;
}

gnc_numeric gncDonorGetWorkday (const GncDonor *donor)
{
    if (!donor) return gnc_numeric_zero();
    return donor->workday;
}

gnc_numeric gncDonorGetRate (const GncDonor *donor)
{
    if (!donor) return gnc_numeric_zero();
    return donor->rate;
}

gnc_commodity * gncDonorGetCurrency (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->currency;
}

gboolean gncDonorGetActive (const GncDonor *donor)
{
    if (!donor) return FALSE;
    return donor->active;
}

Account * gncDonorGetCCard (const GncDonor *donor)
{
    if (!donor) return NULL;
    return donor->ccard_acc;
}

gboolean gncDonorIsDirty (const GncDonor *donor)
{
    if (!donor) return FALSE;
    return (qof_instance_get_dirty_flag(donor)
            || gncAddressIsDirty (donor->addr));
}

void gncDonorBeginEdit (GncDonor *donor)
{
    qof_begin_edit(&donor->inst);
}

static void gncDonorOnError (QofInstance *donor, QofBackendError errcode)
{
    PERR("Donor QofBackend Failure: %d", errcode);
    gnc_engine_signal_commit_error( errcode );
}

static void gncDonorOnDone (QofInstance *inst)
{
    GncDonor *donor = (GncDonor *) inst;
    gncAddressClearDirty (donor->addr);
}

static void emp_free (QofInstance *inst)
{
    GncDonor *donor = (GncDonor *) inst;
    gncDonorFree (donor);
}


void gncDonorCommitEdit (GncDonor *donor)
{
    if (!qof_commit_edit (QOF_INSTANCE(donor))) return;
    qof_commit_edit_part2 (&donor->inst, gncDonorOnError,
                           gncDonorOnDone, emp_free);
}

/* ============================================================== */
/* Other functions */

int gncDonorCompare (const GncDonor *a, const GncDonor *b)
{
    if (!a && !b) return 0;
    if (!a && b) return 1;
    if (a && !b) return -1;

    return(strcmp(a->username, b->username));
}

gboolean gncDonorEqual(const GncDonor* a, const GncDonor* b)
{
    if (a == NULL && b == NULL) return TRUE;
    if (a == NULL || b == NULL ) return FALSE;

    g_return_val_if_fail(GNC_IS_DONOR(a), FALSE);
    g_return_val_if_fail(GNC_IS_DONOR(b), FALSE);

    if (g_strcmp0(a->id, b->id) != 0)
    {
        PWARN("IDs differ: %s vs %s", a->id, b->id);
        return FALSE;
    }

    if (g_strcmp0(a->username, b->username) != 0)
    {
        PWARN("Usernames differ: %s vs %s", a->username, b->username);
        return FALSE;
    }

    if (!gncAddressEqual(a->addr, b->addr))
    {
        PWARN("Addresses differ");
        return FALSE;
    }

    if (!gnc_commodity_equal(a->currency, b->currency))
    {
        PWARN("Currencies differ");
        return FALSE;
    }

    if (a->active != b->active)
    {
        PWARN("Active flags differ");
        return FALSE;
    }

    if (g_strcmp0(a->language, b->language) != 0)
    {
        PWARN("Languages differ: %s vs %s", a->language, b->language);
        return FALSE;
    }

    if (g_strcmp0(a->acl, b->acl) != 0)
    {
        PWARN("ACLs differ: %s vs %s", a->acl, b->acl);
        return FALSE;
    }

    if (!xaccAccountEqual(a->ccard_acc, b->ccard_acc, TRUE))
    {
        PWARN("Accounts differ");
        return FALSE;
    }

    if (!gnc_numeric_equal(a->workday, b->workday))
    {
        PWARN("Workdays differ");
        return FALSE;
    }

    if (!gnc_numeric_equal(a->rate, b->rate))
    {
        PWARN("Rates differ");
        return FALSE;
    }

    return TRUE;
}

/* Package-Private functions */

static const char * _gncDonorPrintable (gpointer item)
{
    GncDonor *v = item;
    if (!item) return NULL;
    return gncAddressGetName(v->addr);
}

/**
 * Listen for qof events.
 *
 * - If the address of an donor has changed, mark the donor as dirty.
 * - If a lot related to an donor has changed, clear the donor's
 *   cached balance as it likely has become invalid.
 *
 * @param entity Entity for the event
 * @param event_type Event type
 * @param user_data User data registered with the handler
 * @param event_data Event data passed with the event.
 */
static void
donor_handle_qof_events (QofInstance *entity, QofEventId event_type,
                        gpointer user_data, gpointer event_data)
{

    /* Handle address change events */
    if ((GNC_IS_ADDRESS (entity) &&
        (event_type & QOF_EVENT_MODIFY) != 0))
    {
        if (GNC_IS_DONOR (event_data))
        {
            GncDonor* empl = GNC_DONOR (event_data);
            gncDonorBeginEdit (empl);
            mark_donor (empl);
            gncDonorCommitEdit (empl);
        }
        return;
    }

    /* Handle lot change events */
    if (GNC_IS_LOT (entity))
    {
        GNCLot *lot = GNC_LOT (entity);
        GncOwner lot_owner;
        const GncOwner *end_owner = NULL;
        GncInvoice *invoice = gncInvoiceGetInvoiceFromLot (lot);

        /* Determine the owner associated with the lot */
        if (invoice)
            /* Invoice lots */
            end_owner = gncOwnerGetEndOwner (gncInvoiceGetOwner (invoice));
        else if (gncOwnerGetOwnerFromLot (lot, &lot_owner))
            /* Pre-payment lots */
            end_owner = gncOwnerGetEndOwner (&lot_owner);

        if (gncOwnerGetType (end_owner) == GNC_OWNER_DONOR)
        {
            /* Clear the cached balance */
            GncDonor* empl = gncOwnerGetDonor (end_owner);
            g_free (empl->balance);
            empl->balance = NULL;
        }
        return;
    }
}

static void
destroy_donor_on_book_close(QofInstance *ent, gpointer data)
{
    GncDonor* e = GNC_DONOR(ent);

    gncDonorBeginEdit(e);
    gncDonorDestroy(e);
}

/** Handles book end - frees all donors from the book
 *
 * @param book Book being closed
 */
static void
gnc_donor_book_end(QofBook* book)
{
    QofCollection *col;

    col = qof_book_get_collection(book, GNC_ID_DONOR);
    qof_collection_foreach(col, destroy_donor_on_book_close, NULL);
}

static QofObject gncDonorDesc =
{
    DI(.interface_version = ) QOF_OBJECT_VERSION,
    DI(.e_type            = ) _GNC_MOD_NAME,
    DI(.type_label        = ) "Donor",
    DI(.create            = ) (gpointer)gncDonorCreate,
    DI(.book_begin        = ) NULL,
    DI(.book_end          = ) gnc_donor_book_end,
    DI(.is_dirty          = ) qof_collection_is_dirty,
    DI(.mark_clean        = ) qof_collection_mark_clean,
    DI(.foreach           = ) qof_collection_foreach,
    DI(.printable         = ) _gncDonorPrintable,
    DI(.version_cmp       = ) (int (*)(gpointer, gpointer)) qof_instance_version_cmp,
};

gboolean gncDonorRegister (void)
{
    static QofParam params[] =
    {
        { DONOR_ID, QOF_TYPE_NATURAL, (QofAccessFunc)gncDonorGetID, (QofSetterFunc)gncDonorSetID },
        {
            DONOR_USERNAME, QOF_TYPE_STRING, (QofAccessFunc)gncDonorGetUsername,
            (QofSetterFunc)gncDonorSetUsername
        },
        {
            DONOR_NAME, QOF_TYPE_STRING, (QofAccessFunc)gncDonorGetName,
            (QofSetterFunc)gncDonorSetName
        },
        {
            DONOR_LANGUAGE, QOF_TYPE_STRING, (QofAccessFunc)gncDonorGetLanguage,
            (QofSetterFunc)gncDonorSetLanguage
        },
        { DONOR_ACL, QOF_TYPE_STRING, (QofAccessFunc)gncDonorGetAcl, (QofSetterFunc)gncDonorSetAcl },
        {
            DONOR_WORKDAY, QOF_TYPE_NUMERIC, (QofAccessFunc)gncDonorGetWorkday,
            (QofSetterFunc)gncDonorSetWorkday
        },
        { DONOR_RATE, QOF_TYPE_NUMERIC, (QofAccessFunc)gncDonorGetRate, (QofSetterFunc)gncDonorSetRate },
        { DONOR_ADDR, GNC_ID_ADDRESS, (QofAccessFunc)gncDonorGetAddr, (QofSetterFunc)qofDonorSetAddr },
        { DONOR_CC,  GNC_ID_ACCOUNT, (QofAccessFunc)gncDonorGetCCard, (QofSetterFunc)gncDonorSetCCard },
        { QOF_PARAM_ACTIVE, QOF_TYPE_BOOLEAN, (QofAccessFunc)gncDonorGetActive, (QofSetterFunc)gncDonorSetActive },
        { QOF_PARAM_BOOK, QOF_ID_BOOK, (QofAccessFunc)qof_instance_get_book, NULL },
        { QOF_PARAM_GUID, QOF_TYPE_GUID, (QofAccessFunc)qof_instance_get_guid, NULL },
        { NULL },
    };

    qof_class_register (_GNC_MOD_NAME, (QofSortFunc)gncDonorCompare, params);

    return qof_object_register (&gncDonorDesc);
}


/* Nonprofit-specific fields stored in KVP for forward compatibility */
void gncDonorSetAnonymous (GncDonor *donor, gboolean anonymous)
{
    GValue v = G_VALUE_INIT;
    if (!donor) return;
    gncDonorBeginEdit (donor);
    g_value_init (&v, G_TYPE_BOOLEAN);
    g_value_set_boolean (&v, anonymous);
    qof_instance_set_kvp (QOF_INSTANCE (donor), &v, 1, DONOR_ANONYMOUS);
    g_value_unset (&v);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

gboolean gncDonorGetAnonymous (const GncDonor *donor)
{
    GValue v = G_VALUE_INIT;
    gboolean anon = FALSE;
    if (!donor) return FALSE;
    g_value_init (&v, G_TYPE_BOOLEAN);
    qof_instance_get_kvp (QOF_INSTANCE (donor), &v, 1, DONOR_ANONYMOUS);
    if (G_VALUE_HOLDS_BOOLEAN (&v))
        anon = g_value_get_boolean (&v);
    g_value_unset (&v);
    return anon;
}

void gncDonorSetEin (GncDonor *donor, const char *ein)
{
    GValue v = G_VALUE_INIT;
    if (!donor) return;
    gncDonorBeginEdit (donor);
    g_value_init (&v, G_TYPE_STRING);
    g_value_set_string (&v, ein ? ein : "");
    qof_instance_set_kvp (QOF_INSTANCE (donor), &v, 1, DONOR_EIN);
    g_value_unset (&v);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

const char * gncDonorGetEin (const GncDonor *donor)
{
    GValue v = G_VALUE_INIT;
    const char *ein = NULL;
    if (!donor) return NULL;
    g_value_init (&v, G_TYPE_STRING);
    qof_instance_get_kvp (QOF_INSTANCE (donor), &v, 1, DONOR_EIN);
    if (G_VALUE_HOLDS_STRING (&v))
        ein = g_value_get_string (&v);
    /* Note: string owned by GValue until unset — copy into static? Prefer strdup pattern used elsewhere.
       For simplicity return interned empty if missing. */
    if (!ein)
    {
        g_value_unset (&v);
        return "";
    }
    /* Leak-avoid: return pointer only valid until next KVP get; callers should copy. */
    {
        static GPrivate key = G_PRIVATE_INIT (g_free);
        gchar *copy = g_strdup (ein);
        g_free (g_private_get (&key));
        g_private_set (&key, copy);
        g_value_unset (&v);
        return copy;
    }
}

void gncDonorSetNotes (GncDonor *donor, const char *notes)
{
    GValue v = G_VALUE_INIT;
    if (!donor) return;
    gncDonorBeginEdit (donor);
    g_value_init (&v, G_TYPE_STRING);
    g_value_set_string (&v, notes ? notes : "");
    qof_instance_set_kvp (QOF_INSTANCE (donor), &v, 1, DONOR_NOTES);
    g_value_unset (&v);
    mark_donor (donor);
    gncDonorCommitEdit (donor);
}

const char * gncDonorGetNotes (const GncDonor *donor)
{
    GValue v = G_VALUE_INIT;
    const char *notes = NULL;
    if (!donor) return NULL;
    g_value_init (&v, G_TYPE_STRING);
    qof_instance_get_kvp (QOF_INSTANCE (donor), &v, 1, DONOR_NOTES);
    if (G_VALUE_HOLDS_STRING (&v))
        notes = g_value_get_string (&v);
    if (!notes)
    {
        g_value_unset (&v);
        return "";
    }
    {
        static GPrivate key = G_PRIVATE_INIT (g_free);
        gchar *copy = g_strdup (notes);
        g_free (g_private_get (&key));
        g_private_set (&key, copy);
        g_value_unset (&v);
        return copy;
    }
}

gchar *gncDonorNextID (QofBook *book)
{
    return qof_book_increment_and_format_counter (book, _GNC_MOD_NAME);
}

const gnc_numeric*
gncDonorGetCachedBalance (GncDonor *empl)
{
    return empl->balance;
}

void gncDonorSetCachedBalance (GncDonor *empl, const gnc_numeric *new_bal)
{
    if (!new_bal)
    {
        if (empl->balance)
        {
            g_free (empl->balance);
            empl->balance = NULL;
        }
        return;
    }

    if (!empl->balance)
        empl->balance = g_new0 (gnc_numeric, 1);

    *empl->balance = *new_bal;
}
