/********************************************************************\
 * gnc-donor-sql.c -- donor sql implementation                *
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

/** @file gnc-donor-sql.c
 *  @brief load and save address data to SQL
 *  @author Copyright (c) 2007-2008 Phil Longstaff <plongstaff@rogers.com>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring data to/from an SQL database
 */
#include <glib.h>

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "gnc-commodity.h"
#include "gncDonorP.h"

#include "gnc-sql-connection.hpp"
#include "gnc-sql-backend.hpp"
#include "gnc-sql-object-backend.hpp"
#include "gnc-sql-column-table-entry.hpp"
#include "gnc-slots-sql.h"
#include "gnc-commodity-sql.h"
#include "gnc-donor-sql.h"

#define _GNC_MOD_NAME   GNC_ID_DONOR

static QofLogModule log_module = G_LOG_DOMAIN;

#define MAX_USERNAME_LEN 2048
#define MAX_ID_LEN 2048
#define MAX_LANGUAGE_LEN 2048
#define MAX_ACL_LEN 2048

#define TABLE_NAME "donors"
#define TABLE_VERSION 2

static EntryVec col_table
({
    gnc_sql_make_table_entry<CT_GUID>("guid", 0, COL_NNUL | COL_PKEY, "guid"),
    gnc_sql_make_table_entry<CT_STRING>(
        "username", MAX_USERNAME_LEN, COL_NNUL, "username"),
    gnc_sql_make_table_entry<CT_STRING>("id", MAX_ID_LEN, COL_NNUL, "id"),
    gnc_sql_make_table_entry<CT_STRING>(
        "language", MAX_LANGUAGE_LEN, COL_NNUL, "language"),
    gnc_sql_make_table_entry<CT_STRING>("acl", MAX_ACL_LEN, COL_NNUL, "acl"),
    gnc_sql_make_table_entry<CT_BOOLEAN>("active", 0, COL_NNUL, "active"),
    gnc_sql_make_table_entry<CT_COMMODITYREF>(
        "currency", 0, COL_NNUL, "currency"),
    gnc_sql_make_table_entry<CT_ACCOUNTREF>(
        "ccard_guid", 0, 0, "credit-card-account"),
    gnc_sql_make_table_entry<CT_NUMERIC>("workday", 0, COL_NNUL, "workday"),
    gnc_sql_make_table_entry<CT_NUMERIC>("rate", 0, COL_NNUL, "rate"),
    gnc_sql_make_table_entry<CT_ADDRESS>("addr", 0, 0, "address"),
});

GncSqlDonorBackend::GncSqlDonorBackend() :
    GncSqlObjectBackend(TABLE_VERSION, GNC_ID_DONOR,
                        TABLE_NAME, col_table) {}

static GncDonor*
load_single_donor (GncSqlBackend* sql_be, GncSqlRow& row)
{
    const GncGUID* guid;
    GncDonor* pDonor;

    g_return_val_if_fail (sql_be != NULL, NULL);

    guid = gnc_sql_load_guid (sql_be, row);
    pDonor = gncDonorLookup (sql_be->book(), guid);
    if (pDonor == NULL)
    {
        pDonor = gncDonorCreate (sql_be->book());
    }
    gnc_sql_load_object (sql_be, row, GNC_ID_DONOR, pDonor, col_table);
    qof_instance_mark_clean (QOF_INSTANCE (pDonor));

    return pDonor;
}
/* Because gncCustomerLookup has the arguments backwards: */
static inline GncDonor*
gnc_donor_lookup (const GncGUID *guid, const QofBook *book)
{
    QOF_BOOK_RETURN_ENTITY(book, guid, GNC_ID_DONOR, GncDonor);
}

void
GncSqlDonorBackend::load_all (GncSqlBackend* sql_be)
{
    g_return_if_fail (sql_be != NULL);

    std::string sql("SELECT * FROM " TABLE_NAME);
    auto stmt = sql_be->create_statement_from_sql(sql);
    auto result = sql_be->execute_select_statement(stmt);

    for (auto row : *result)
        load_single_donor (sql_be, row);

    std::string pkey(col_table[0]->name());
    sql = "SELECT DISTINCT ";
    sql += pkey + " FROM " TABLE_NAME;
    gnc_sql_slots_load_for_sql_subquery (sql_be, sql,
					 (BookLookupFn)gnc_donor_lookup);
}


/* ================================================================= */
void
GncSqlDonorBackend::create_tables (GncSqlBackend* sql_be)
{
    gint version;

    g_return_if_fail (sql_be != NULL);

    version = sql_be->get_table_version( TABLE_NAME);
    if (version == 0)
    {
        sql_be->create_table(TABLE_NAME, TABLE_VERSION, col_table);
    }
    else if (version < m_version)
    {
        /* Upgrade 64 bit int handling */
        sql_be->upgrade_table(TABLE_NAME, col_table);
        sql_be->set_table_version (TABLE_NAME, TABLE_VERSION);

        PINFO ("Donors table upgraded from version 1 to version %d\n",
               TABLE_VERSION);
    }
}

/* ================================================================= */
bool
GncSqlDonorBackend::commit (GncSqlBackend* sql_be, QofInstance* inst)
{
    GncDonor* emp;
    const GncGUID* guid;
    E_DB_OPERATION op;
    gboolean is_infant;
    gboolean is_ok = TRUE;

    g_return_val_if_fail (inst != NULL, FALSE);
    g_return_val_if_fail (GNC_IS_DONOR (inst), FALSE);
    g_return_val_if_fail (sql_be != NULL, FALSE);

    emp = GNC_DONOR (inst);

    is_infant = qof_instance_get_infant (inst);
    if (qof_instance_get_destroying (inst))
    {
        op = OP_DB_DELETE;
    }
    else if (sql_be->pristine() || is_infant)
    {
        op = OP_DB_INSERT;
    }
    else
    {
        op = OP_DB_UPDATE;
    }
    if (op != OP_DB_DELETE)
    {
        // Ensure the commodity is in the db
        is_ok = sql_be->save_commodity(gncDonorGetCurrency (emp));
    }

    if (is_ok)
    {
        is_ok = sql_be->do_db_operation(op, TABLE_NAME, GNC_ID_DONOR, emp,
                                        col_table);
    }

    if (is_ok)
    {
        // Now, commit or delete any slots
        guid = qof_instance_get_guid (inst);
        if (!qof_instance_get_destroying (inst))
        {
            is_ok = gnc_sql_slots_save (sql_be, guid, is_infant, inst);
        }
        else
        {
            is_ok = gnc_sql_slots_delete (sql_be, guid);
        }
    }

    return is_ok;
}

/* ================================================================= */
static gboolean
donor_should_be_saved (GncDonor* donor)
{
    const char* id;

    g_return_val_if_fail (donor != NULL, FALSE);

    /* make sure this is a valid donor before we save it -- should have an ID */
    id = gncDonorGetID (donor);
    if (id == NULL || *id == '\0')
    {
        return FALSE;
    }

    return TRUE;
}

static void
write_single_donor (QofInstance* term_p, gpointer data_p)
{
    write_objects_t* s = (write_objects_t*)data_p;

    g_return_if_fail (term_p != NULL);
    g_return_if_fail (GNC_IS_DONOR (term_p));
    g_return_if_fail (data_p != NULL);

    if (s->is_ok && donor_should_be_saved (GNC_DONOR (term_p)))
    {
        s->is_ok = s->obe->commit (s->be, term_p);
    }
}

bool
GncSqlDonorBackend::write (GncSqlBackend* sql_be)
{
    write_objects_t data;

    g_return_val_if_fail (sql_be != NULL, FALSE);

    data.be = sql_be;
    data.is_ok = TRUE;
    data.obe = this;
    qof_object_foreach (GNC_ID_DONOR, sql_be->book(), write_single_donor, &data);

    return data.is_ok;
}


/* ========================== END OF FILE ===================== */
