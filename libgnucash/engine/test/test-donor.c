/*********************************************************************
 * test-donor.c
 * Test the donor object.
 *
 * Copyright (c) 2001 Derek Atkins <warlord@MIT.EDU>
 * Copyright (c) 2005 Neil Williams <linux@codehelp.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 *
 *********************************************************************/

#include <config.h>
#include <glib.h>
#include <qof.h>
#include <qofinstance-p.h>

#include "gncDonorP.h"
#include "gncCustomerP.h"
#include "gncJobP.h"
#include "gncInvoiceP.h"
#include "test-stuff.h"

static int count = 0;

static void
test_string_fcn (QofBook *book, const char *message,
                 void (*set) (GncDonor *, const char *str),
                 const char * (*get)(const GncDonor *));

static void
test_numeric_fcn (QofBook *book, const char *message,
                  void (*set) (GncDonor *, gnc_numeric),
                  gnc_numeric (*get)(const GncDonor *));

static void
test_bool_fcn (QofBook *book, const char *message,
               void (*set) (GncDonor *, gboolean),
               gboolean (*get) (const GncDonor *));

#if 0
static void
test_gint_fcn (QofBook *book, const char *message,
               void (*set) (GncDonor *, gint),
               gint (*get) (GncDonor *));
#endif

static void
test_donor (void)
{
    QofBook *book;
    GncDonor *donor;

    book = qof_book_new();

    /* Test creation/destruction */
    {
        do_test (gncDonorCreate (NULL) == NULL, "donor create NULL");
        donor = gncDonorCreate (book);
        do_test (donor != NULL, "donor create");
        do_test (qof_instance_get_book(QOF_INSTANCE(donor)) == book,
                 "getbook");

        gncDonorBeginEdit (donor);
        gncDonorDestroy (donor);
        success ("create/destroy");
    }

    /* Test setting/getting routines; does the active flag get set right? */
    {
        GncGUID guid;

        test_string_fcn (book, "Id", gncDonorSetID, gncDonorGetID);
        test_string_fcn (book, "Username", gncDonorSetUsername, gncDonorGetUsername);
        test_string_fcn (book, "Language", gncDonorSetLanguage, gncDonorGetLanguage);
        test_string_fcn (book, "Acl", gncDonorSetAcl, gncDonorGetAcl);

        test_numeric_fcn (book, "Workday", gncDonorSetWorkday, gncDonorGetWorkday);
        test_numeric_fcn (book, "Rate", gncDonorSetRate, gncDonorGetRate);

        test_bool_fcn (book, "Active", gncDonorSetActive, gncDonorGetActive);

        guid_replace (&guid);
        donor = gncDonorCreate (book);
        count++;

        do_test (gncDonorGetAddr (donor) != NULL, "Addr");

        gncDonorSetGUID (donor, &guid);
        do_test (guid_equal (&guid, qof_instance_get_guid(QOF_INSTANCE(donor))), "guid compare");
    }
    {
        GList *list;

        list = gncBusinessGetList (book, GNC_ID_DONOR, TRUE);
        do_test (list != NULL, "getList all");
        do_test (g_list_length (list) == count, "correct length: all");
        g_list_free (list);

        list = gncBusinessGetList (book, GNC_ID_DONOR, FALSE);
        do_test (list != NULL, "getList active");
        do_test (g_list_length (list) == 1, "correct length: active");
        g_list_free (list);
    }
    {
        char *str = get_random_string();

        GncAddress *addr = gncDonorGetAddr (donor);
        gncAddressSetName (addr, str);
        const char *res = qof_object_printable (GNC_ID_DONOR, donor);
        do_test (res != NULL, "Printable NULL?");
        do_test (g_strcmp0 (str, res) == 0, "Printable equals");
        g_free (str);
    }

    qof_book_destroy (book);
}

static void
test_string_fcn (QofBook *book, const char *message,
                 void (*set) (GncDonor *, const char *str),
                 const char * (*get)(const GncDonor *))
{
    GncDonor *donor = gncDonorCreate (book);
    char *str = get_random_string ();

    do_test (!gncDonorIsDirty (donor), "test if start dirty");
    gncDonorBeginEdit (donor);
    set (donor, str);
    /* Donor record should be dirty */
    do_test (gncDonorIsDirty (donor), "test dirty later");
    gncDonorCommitEdit (donor);
    /* Donor record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDonorIsDirty (donor), "test dirty after commit");
    do_test (g_strcmp0 (get (donor), str) == 0, message);
    g_free (str);
    gncDonorSetActive (donor, FALSE);
    count++;
}

static void
test_numeric_fcn (QofBook *book, const char *message,
                  void (*set) (GncDonor *, gnc_numeric),
                  gnc_numeric (*get)(const GncDonor *))
{
    GncDonor *donor = gncDonorCreate (book);
    gnc_numeric num = gnc_numeric_create (17, 1);

    do_test (!gncDonorIsDirty (donor), "test if start dirty");
    gncDonorBeginEdit (donor);
    set (donor, num);
    /* Donor record should be dirty */
    do_test (gncDonorIsDirty (donor), "test dirty later");
    gncDonorCommitEdit (donor);
    /* Donor record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDonorIsDirty (donor), "test dirty after commit");
    do_test (gnc_numeric_equal (get (donor), num), message);
    gncDonorSetActive (donor, FALSE);
    count++;
}

static void
test_bool_fcn (QofBook *book, const char *message,
               void (*set) (GncDonor *, gboolean),
               gboolean (*get) (const GncDonor *))
{
    GncDonor *donor = gncDonorCreate (book);
    gboolean num = get_random_boolean ();

    do_test (!gncDonorIsDirty (donor), "test if start dirty");
    gncDonorBeginEdit (donor);
    set (donor, FALSE);
    set (donor, TRUE);
    set (donor, num);
    /* Donor record should be dirty */
    do_test (gncDonorIsDirty (donor), "test dirty later");
    gncDonorCommitEdit (donor);
    /* Donor record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDonorIsDirty (donor), "test dirty after commit");
    do_test (get (donor) == num, message);
    gncDonorSetActive (donor, FALSE);
    count++;
}

#if 0
static void
test_gint_fcn (QofBook *book, const char *message,
               void (*set) (GncDonor *, gint),
               gint (*get) (GncDonor *))
{
    GncDonor *donor = gncDonorCreate (book);
    gint num = 17;

    do_test (!gncDonorIsDirty (donor), "test if start dirty");
    gncDonorBeginEdit (donor);
    set (donor, num);
    /* Donor record should be dirty */
    do_test (gncDonorIsDirty (donor), "test dirty later");
    gncDonorCommitEdit (donor);
    /* Donor record should be not dirty */
    /* Skip, because will always fail without a backend.
     * It's not possible to load a backend in the engine code
     * without having circular dependencies.
     */
    // do_test (!gncDonorIsDirty (donor), "test dirty after commit");
    do_test (get (donor) == num, message);
    gncDonorSetActive (donor, FALSE);
    count++;
}
#endif

int
main (int argc, char **argv)
{
    qof_init();
    do_test (gncInvoiceRegister(), "Cannot register GncInvoice");
    do_test (gncJobRegister (),  "Cannot register GncJob");
    do_test (gncCustomerRegister(), "Cannot register GncCustomer");
    do_test (gncDonorRegister(), "Cannot register GncDonor");
    test_donor();
    print_test_results();
    qof_close();
    return get_rv();
}
