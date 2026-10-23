/********************************************************************\
 * gnc-donor-xml-v2.c -- donor xml i/o implementation         *
 *                                                                  *
 * Copyright (C) 2002 Derek Atkins <warlord@MIT.EDU>                *
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
#include <glib.h>

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include "gncDonorP.h"
#include "gnc-xml-helper.h"
#include "sixtp.h"
#include "sixtp-utils.h"
#include "sixtp-parsers.h"
#include "sixtp-utils.h"
#include "sixtp-dom-parsers.h"
#include "sixtp-dom-generators.h"

#include "gnc-xml.h"
#include "io-gncxml-gen.h"
#include "io-gncxml-v2.h"

#include "gnc-donor-xml-v2.h"
#include "gnc-address-xml-v2.h"

#define _GNC_MOD_NAME   GNC_ID_DONOR

static QofLogModule log_module = GNC_MOD_IO;

const gchar* donor_version_string = "2.0.0";

/* ids */
#define gnc_donor_string "gnc:GncDonor"
#define donor_username_string "donor:username"
#define donor_guid_string "donor:guid"
#define donor_id_string "donor:id"
#define donor_addr_string "donor:addr"
#define donor_language_string "donor:language"
#define donor_acl_string "donor:acl"
#define donor_active_string "donor:active"
#define donor_workday_string "donor:workday"
#define donor_rate_string "donor:rate"
#define donor_currency_string "donor:currency"
#define donor_ccard_string "donor:ccard"
#define donor_slots_string "donor:slots"

static void
maybe_add_string (xmlNodePtr ptr, const char* tag, const char* str)
{
    if (str && *str)
        xmlAddChild (ptr, text_to_dom_tree (tag, str));
}

static xmlNodePtr
donor_dom_tree_create (GncDonor* donor)
{
    xmlNodePtr ret;
    gnc_numeric num;
    Account* ccard_acc;

    ret = xmlNewNode (NULL, BAD_CAST gnc_donor_string);
    xmlSetProp (ret, BAD_CAST "version", BAD_CAST donor_version_string);

    xmlAddChild (ret, guid_to_dom_tree (donor_guid_string,
                                        qof_instance_get_guid (QOF_INSTANCE (donor))));

    xmlAddChild (ret, text_to_dom_tree (donor_username_string,
                                        gncDonorGetUsername (donor)));

    xmlAddChild (ret, text_to_dom_tree (donor_id_string,
                                        gncDonorGetID (donor)));

    xmlAddChild (ret, gnc_address_to_dom_tree (donor_addr_string,
                                               gncDonorGetAddr (donor)));

    maybe_add_string (ret, donor_language_string,
                      gncDonorGetLanguage (donor));
    maybe_add_string (ret, donor_acl_string, gncDonorGetAcl (donor));

    xmlAddChild (ret, int_to_dom_tree (donor_active_string,
                                       gncDonorGetActive (donor)));

    num = gncDonorGetWorkday (donor);
    xmlAddChild (ret, gnc_numeric_to_dom_tree (donor_workday_string, &num));

    num = gncDonorGetRate (donor);
    xmlAddChild (ret, gnc_numeric_to_dom_tree (donor_rate_string, &num));

    xmlAddChild
    (ret,
     commodity_ref_to_dom_tree (donor_currency_string,
                                gncDonorGetCurrency (donor)));

    ccard_acc = gncDonorGetCCard (donor);
    if (ccard_acc)
        xmlAddChild (ret, guid_to_dom_tree (donor_ccard_string,
                                            qof_instance_get_guid (QOF_INSTANCE (ccard_acc))));

    /* xmlAddChild won't do anything with a NULL, so tests are superfluous. */
    xmlAddChild (ret, qof_instance_slots_to_dom_tree (donor_slots_string,
                                                      QOF_INSTANCE (donor)));
    return ret;
}

/***********************************************************************/

struct donor_pdata
{
    GncDonor* donor;
    QofBook* book;
};

static gboolean
donor_username_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    return apply_xmlnode_text (gncDonorSetUsername, pdata->donor, node);
}

static gboolean
donor_guid_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);
    GncDonor* donor;

    auto guid = dom_tree_to_guid (node);
    g_return_val_if_fail (guid, FALSE);

    /* See if we've already created this one */
    donor = gncDonorLookup (pdata->book, &*guid);
    if (donor)
    {
        gncDonorDestroy (pdata->donor);
        pdata->donor = donor;
        gncDonorBeginEdit (donor);
    }
    else
    {
        gncDonorSetGUID (pdata->donor, &*guid);
    }

    return TRUE;
}

static gboolean
donor_id_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    return apply_xmlnode_text (gncDonorSetID, pdata->donor, node);
}

static gboolean
donor_language_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    return apply_xmlnode_text (gncDonorSetLanguage, pdata->donor, node);
}

static gboolean
donor_acl_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    return apply_xmlnode_text (gncDonorSetAcl, pdata->donor, node);
}

static gboolean
donor_addr_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    return gnc_dom_tree_to_address (node, gncDonorGetAddr (pdata->donor));
}

static gboolean
donor_active_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);
    gint64 val;
    gboolean ret;

    ret = dom_tree_to_integer (node, &val);
    if (ret)
        gncDonorSetActive (pdata->donor, (gboolean)val);

    return ret;
}

static gboolean
donor_workday_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    gncDonorSetWorkday (pdata->donor, dom_tree_to_gnc_numeric (node));
    return TRUE;
}

static gboolean
donor_rate_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);

    gncDonorSetRate (pdata->donor, dom_tree_to_gnc_numeric (node));
    return TRUE;
}

static gboolean
donor_currency_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);
    gnc_commodity* com;

    com = dom_tree_to_commodity_ref (node, pdata->book);
    g_return_val_if_fail (com, FALSE);

    gncDonorSetCurrency (pdata->donor, com);

    return TRUE;
}

static gboolean
donor_ccard_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);
    Account* ccard_acc;

    auto guid = dom_tree_to_guid (node);
    g_return_val_if_fail (guid, FALSE);

    ccard_acc = xaccAccountLookup (&*guid, pdata->book);

    g_return_val_if_fail (ccard_acc, FALSE);
    gncDonorSetCCard (pdata->donor, ccard_acc);

    return TRUE;
}

static gboolean
donor_slots_handler (xmlNodePtr node, gpointer donor_pdata)
{
    struct donor_pdata* pdata = static_cast<decltype (pdata)> (donor_pdata);
    return dom_tree_create_instance_slots (node, QOF_INSTANCE (pdata->donor));
}

static struct dom_tree_handler donor_handlers_v2[] =
{
    { donor_username_string, donor_username_handler, 1, 0 },
    { donor_guid_string, donor_guid_handler, 1, 0 },
    { donor_id_string, donor_id_handler, 1, 0 },
    { donor_addr_string, donor_addr_handler, 1, 0 },
    { donor_language_string, donor_language_handler, 0, 0 },
    { donor_acl_string, donor_acl_handler, 0, 0 },
    { donor_active_string, donor_active_handler, 1, 0 },
    { donor_workday_string, donor_workday_handler, 1, 0 },
    { donor_rate_string, donor_rate_handler, 1, 0 },
    { donor_currency_string, donor_currency_handler, 0, 0 }, /* XXX */
    { "donor:commodity", donor_currency_handler, 0, 0 }, /* XXX */
    { donor_ccard_string, donor_ccard_handler, 0, 0 },
    { donor_slots_string, donor_slots_handler, 0, 0 },
    { NULL, 0, 0, 0 }
};

static GncDonor*
dom_tree_to_donor (xmlNodePtr node, QofBook* book)
{
    struct donor_pdata donor_pdata;
    gboolean successful;

    donor_pdata.donor = gncDonorCreate (book);
    donor_pdata.book = book;
    gncDonorBeginEdit (donor_pdata.donor);

    successful = dom_tree_generic_parse (node, donor_handlers_v2,
                                         &donor_pdata);
    if (successful)
        gncDonorCommitEdit (donor_pdata.donor);
    else
    {
        PERR ("failed to parse donor tree");
        gncDonorDestroy (donor_pdata.donor);
        donor_pdata.donor = NULL;
    }

    return donor_pdata.donor;
}

static gboolean
gnc_donor_end_handler (gpointer data_for_children,
                          GSList* data_from_children, GSList* sibling_data,
                          gpointer parent_data, gpointer global_data,
                          gpointer* result, const gchar* tag)
{
    GncDonor* donor;
    xmlNodePtr tree = (xmlNodePtr)data_for_children;
    gxpf_data* gdata = (gxpf_data*)global_data;
    QofBook* book = static_cast<decltype (book)> (gdata->bookdata);


    if (parent_data)
    {
        return TRUE;
    }

    /* OK.  For some messed up reason this is getting called again with a
       NULL tag.  So we ignore those cases */
    if (!tag)
    {
        return TRUE;
    }

    g_return_val_if_fail (tree, FALSE);

    donor = dom_tree_to_donor (tree, book);
    if (donor != NULL)
    {
        gdata->cb (tag, gdata->parsedata, donor);
    }

    xmlFreeNode (tree);

    return donor != NULL;
}

static sixtp*
donor_sixtp_parser_create (void)
{
    return sixtp_dom_parser_new (gnc_donor_end_handler, NULL, NULL);
}

static gboolean
donor_should_be_saved (GncDonor* donor)
{
    const char* id;

    /* make sure this is a valid donor before we save it -- should have an ID */
    id = gncDonorGetID (donor);
    if (id == NULL || *id == '\0')
        return FALSE;

    return TRUE;
}

static void
do_count (QofInstance* donor_p, gpointer count_p)
{
    int* count = static_cast<decltype (count)> (count_p);
    if (donor_should_be_saved ((GncDonor*) donor_p))
        (*count)++;
}

static int
donor_get_count (QofBook* book)
{
    int count = 0;
    qof_object_foreach (_GNC_MOD_NAME, book, do_count, (gpointer) &count);
    return count;
}

static void
xml_add_donor (QofInstance* donor_p, gpointer out_p)
{
    xmlNodePtr node;
    GncDonor* donor = (GncDonor*) donor_p;
    FILE* out = static_cast<decltype (out)> (out_p);

    if (ferror (out))
        return;
    if (!donor_should_be_saved (donor))
        return;

    node = donor_dom_tree_create (donor);
    xmlElemDump (out, NULL, node);
    xmlFreeNode (node);
    if (ferror (out) || fprintf (out, "\n") < 0)
        return;
}

static gboolean
donor_write (FILE* out, QofBook* book)
{
    qof_object_foreach_sorted (_GNC_MOD_NAME, book, xml_add_donor,
                               (gpointer) out);
    return ferror (out) == 0;
}

static gboolean
donor_ns (FILE* out)
{
    g_return_val_if_fail (out, FALSE);
    return gnc_xml2_write_namespace_decl (out, "donor");
}

void
gnc_donor_xml_initialize (void)
{
    static GncXmlDataType_t be_data =
    {
        GNC_FILE_BACKEND_VERS,
        gnc_donor_string,
        donor_sixtp_parser_create,
        NULL,           /* add_item */
        donor_get_count,
        donor_write,
        NULL,           /* scrub */
        donor_ns,
    };

    gnc_xml_register_backend (be_data);
}
