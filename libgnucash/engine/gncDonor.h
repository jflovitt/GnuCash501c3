/********************************************************************\
 * gncDonor.h -- the Core Donor Interface                     *
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
/** @addtogroup Business
    @{ */
/** @addtogroup Donor
    @{ */
/** @file gncDonor.h
    @brief Donor Interface
    @author Copyright (C) 2001 Derek Atkins <warlord@MIT.EDU>
*/

#ifndef GNC_DONOR_H_
#define GNC_DONOR_H_

typedef struct _gncDonor GncDonor;
typedef struct _gncDonorClass GncDonorClass;

#include "gncAddress.h"
#include "Account.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GNC_ID_DONOR "gncDonor"

/* --- type macros --- */
#define GNC_TYPE_DONOR            (gnc_donor_get_type ())
#define GNC_DONOR(o)              \
     (G_TYPE_CHECK_INSTANCE_CAST ((o), GNC_TYPE_DONOR, GncDonor))
#define GNC_DONOR_CLASS(k)        \
     (G_TYPE_CHECK_CLASS_CAST((k), GNC_TYPE_DONOR, GncDonorClass))
#define GNC_IS_DONOR(o)           \
     (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNC_TYPE_DONOR))
#define GNC_IS_DONOR_CLASS(k)     \
     (G_TYPE_CHECK_CLASS_TYPE ((k), GNC_TYPE_DONOR))
#define GNC_DONOR_GET_CLASS(o)    \
     (G_TYPE_INSTANCE_GET_CLASS ((o), GNC_TYPE_DONOR, GncDonorClass))
GType gnc_donor_get_type(void);

/** @name Create/Destroy Functions
 @{ */
GncDonor *gncDonorCreate (QofBook *book);
void gncDonorDestroy (GncDonor *donor);
void gncDonorBeginEdit (GncDonor *donor);
void gncDonorCommitEdit (GncDonor *donor);
int gncDonorCompare (const GncDonor *a, const GncDonor *b);
/** @} */

/** @name Set Functions
 @{ */
void gncDonorSetID (GncDonor *donor, const char *id);
void gncDonorSetUsername (GncDonor *donor, const char *username);
/* Note: Donors don't have a name property defined, but
 * in order to get a consistent interface with other owner types,
 * this function fakes one by setting the name property of
 * the donor's address.
 */
void gncDonorSetName (GncDonor *donor, const char *name);
void gncDonorSetLanguage (GncDonor *donor, const char *language);
void gncDonorSetAcl (GncDonor *donor, const char *acl);
void gncDonorSetWorkday (GncDonor *donor, gnc_numeric workday);
void gncDonorSetRate (GncDonor *donor, gnc_numeric rate);
void gncDonorSetCurrency (GncDonor *donor, gnc_commodity * currency);
void gncDonorSetActive (GncDonor *donor, gboolean active);
void gncDonorSetCCard (GncDonor *donor, Account* ccard_acc);
void gncDonorSetAnonymous (GncDonor *donor, gboolean anonymous);
void gncDonorSetEin (GncDonor *donor, const char *ein);
void gncDonorSetNotes (GncDonor *donor, const char *notes);
void qofDonorSetAddr (GncDonor *donor, QofInstance *addr_ent);

/** @} */

/** @name Get Functions
 @{ */
QofBook * gncDonorGetBook (GncDonor *donor);
const char * gncDonorGetID (const GncDonor *donor);
const char * gncDonorGetUsername (const GncDonor *donor);
/* Note: Donors don't have a name property defined, but
 * in order to get a consistent interface with other owner types,
 * this function fakes one by returning the name property of
 * the donor's address.
 */
const char * gncDonorGetName (const GncDonor *donor);
GncAddress * gncDonorGetAddr (const GncDonor *donor);
const char * gncDonorGetLanguage (const GncDonor *donor);
const char * gncDonorGetAcl (const GncDonor *donor);
gnc_numeric gncDonorGetWorkday (const GncDonor *donor);
gnc_numeric gncDonorGetRate (const GncDonor *donor);
gnc_commodity * gncDonorGetCurrency (const GncDonor *donor);
gboolean gncDonorGetActive (const GncDonor *donor);
Account * gncDonorGetCCard (const GncDonor *donor);
gboolean gncDonorGetAnonymous (const GncDonor *donor);
const char * gncDonorGetEin (const GncDonor *donor);
const char * gncDonorGetNotes (const GncDonor *donor);
/** @} */


/** Return a pointer to the instance gncDonor that is identified
 *  by the guid, and is residing in the book. Returns NULL if the
 *  instance can't be found.
 */
static inline GncDonor * gncDonorLookup (const QofBook *book, const GncGUID *guid)
{
    QOF_BOOK_RETURN_ENTITY(book, guid, GNC_ID_DONOR, GncDonor);
}


#define DONOR_ID       "id"
#define DONOR_USERNAME "username"
#define DONOR_NAME     "name"
#define DONOR_ADDR     "addr"
#define DONOR_LANGUAGE "native language"
#define DONOR_ACL      "acl"
#define DONOR_WORKDAY  "workday"
#define DONOR_RATE     "rate"
#define DONOR_CC       "credit_card_account"
#define DONOR_ANONYMOUS "anonymous"
#define DONOR_EIN       "ein"
#define DONOR_NOTES     "notes"

/** deprecated routines */
#define gncDonorGetGUID(E) qof_entity_get_guid(QOF_INSTANCE(E))
#define gncDonorGetBook(E) qof_instance_get_book(QOF_INSTANCE(E))
#define gncDonorRetGUID(E) (E ? *(qof_entity_get_guid(QOF_INSTANCE(E))) : *(guid_null()))
#define gncDonorLookupDirect(G,B) gncDonorLookup((B),&(G))

/** Test support function, used by test-dbi-business-stuff.c */
gboolean gncDonorEqual(const GncDonor* e1, const GncDonor* e2);
gboolean gncDonorIsDirty (const GncDonor *donor);

#ifdef __cplusplus
}
#endif

#endif /* GNC_DONOR_H_ */
/** @} */
/** @} */
