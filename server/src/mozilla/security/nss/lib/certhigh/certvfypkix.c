/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * nss_pkix_proxy.h
 *
 * PKIX - NSS proxy functions
 *
 * NOTE: All structures, functions, data types are parts of library private
 * api and are subjects to change in any following releases.
 *
 */
#include "prerror.h"
#include "prprf.h"
 
#include "nspr.h"
#include "pk11func.h"
#include "certdb.h"
#include "cert.h"
#include "secerr.h"
#include "nssb64.h"
#include "secasn1.h"
#include "secder.h"
#include "pkit.h"

#include "pkix_pl_common.h"
#include "pkix_pl_ekuchecker.h"

extern PRLogModuleInfo *pkixLog;

#ifdef DEBUG_volkov
/* Temporary declarations of functioins. Will be removed with fix for
 * 391183 */
extern char *
pkix_Error2ASCII(PKIX_Error *error, void *plContext);

extern void
cert_PrintCert(PKIX_PL_Cert *pkixCert, void *plContext);

extern PKIX_Error *
cert_PrintCertChain(PKIX_List *pkixCertChain, void *plContext);

#endif /* DEBUG */

#ifdef PKIX_OBJECT_LEAK_TEST

extern PKIX_UInt32
pkix_pl_lifecycle_ObjectLeakCheck(int *);

extern SECStatus
pkix_pl_lifecycle_ObjectTableUpdate(int *objCountTable);

PRInt32 parallelFnInvocationCount;

#endif /* PKIX_OBJECT_LEAK_TEST */


static PRBool usePKIXValidationEngine = PR_FALSE;

/*
 * FUNCTION: CERT_SetUsePKIXForValidation
 * DESCRIPTION:
 *
 * Enables or disables use of libpkix for certificate validation
 *
 * PARAMETERS:
 *  "enable"
 *      PR_TRUE: enables use of libpkix for cert validation.
 *      PR_FALSE: disables.
 * THREAD SAFETY:
 *  NOT Thread Safe.
 * RETURNS:
 *  Returns SECSuccess if successfully enabled
 */
SECStatus
CERT_SetUsePKIXForValidation(PRBool enable)
{
    usePKIXValidationEngine = (enable > 0) ? PR_TRUE : PR_FALSE;
    return SECSuccess;
}

/*
 * FUNCTION: CERT_GetUsePKIXForValidation
 * DESCRIPTION:
 *
 * Checks if libpkix building function should be use for certificate
 * chain building.
 *
 * PARAMETERS:
 *  NONE
 * THREAD SAFETY:
 *  NOT Thread Safe
 * RETURNS:
 *  Returns PR_TRUE if libpkix should be used. PR_FALSE otherwise.
 */
PRBool
CERT_GetUsePKIXForValidation()
{
    return usePKIXValidationEngine;
}

/*
 * FUNCTION: cert_NssKeyUsagesToPkix
 * DESCRIPTION:
 *
 * Converts nss key usage bit field(PRUint32) to pkix key usage
 * bit field.
 *
 * PARAMETERS:
 *  "nssKeyUsage"
 *      Nss key usage bit field.
 *  "pkixKeyUsage"
 *      Pkix key usage big field.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_NssKeyUsagesToPkix(
    PRUint32 nssKeyUsage,
    PKIX_UInt32 *pPkixKeyUsage,
    void *plContext)
{
    PKIX_UInt32 pkixKeyUsage = 0;

    PKIX_ENTER(CERTVFYPKIX, "cert_NssKeyUsagesToPkix");
    PKIX_NULLCHECK_ONE(pPkixKeyUsage);

    *pPkixKeyUsage = 0;

    if (nssKeyUsage & KU_DIGITAL_SIGNATURE) {
        pkixKeyUsage |= PKIX_DIGITAL_SIGNATURE;
    }
    
    if (nssKeyUsage & KU_NON_REPUDIATION) {
        pkixKeyUsage |= PKIX_NON_REPUDIATION;
    }

    if (nssKeyUsage & KU_KEY_ENCIPHERMENT) {
        pkixKeyUsage |= PKIX_KEY_ENCIPHERMENT;
    }
    
    if (nssKeyUsage & KU_DATA_ENCIPHERMENT) {
        pkixKeyUsage |= PKIX_DATA_ENCIPHERMENT;
    }
    
    if (nssKeyUsage & KU_KEY_AGREEMENT) {
        pkixKeyUsage |= PKIX_KEY_AGREEMENT;
    }
    
    if (nssKeyUsage & KU_KEY_CERT_SIGN) {
        pkixKeyUsage |= PKIX_KEY_CERT_SIGN;
    }
    
    if (nssKeyUsage & KU_CRL_SIGN) {
        pkixKeyUsage |= PKIX_CRL_SIGN;
    }

    if (nssKeyUsage & KU_ENCIPHER_ONLY) {
        pkixKeyUsage |= PKIX_ENCIPHER_ONLY;
    }
    
    /* Not supported. XXX we should support this once it is
     * fixed in NSS */
    /* pkixKeyUsage |= PKIX_DECIPHER_ONLY; */

    *pPkixKeyUsage = pkixKeyUsage;

    PKIX_RETURN(CERTVFYPKIX);
}

extern char* ekuOidStrings[];

enum {
    ekuIndexSSLServer = 0,
    ekuIndexSSLClient,
    ekuIndexCodeSigner,
    ekuIndexEmail,
    ekuIndexTimeStamp,
    ekuIndexStatusResponder,
    ekuIndexUnknown
} ekuIndex;

typedef struct {
    SECCertUsage certUsage;
    PRUint32 ekuStringIndex;
} SECCertUsageToEku;

const SECCertUsageToEku certUsageEkuStringMap[] = {
    {certUsageSSLClient,             ekuIndexSSLClient},
    {certUsageSSLServer,             ekuIndexSSLServer},
    {certUsageSSLServerWithStepUp,   ekuIndexSSLServer}, /* need to add oids to
                                                          * the list of eku.
                                                          * see 390381*/
    {certUsageSSLCA,                 ekuIndexSSLServer},
    {certUsageEmailSigner,           ekuIndexEmail},
    {certUsageEmailRecipient,        ekuIndexEmail},
    {certUsageObjectSigner,          ekuIndexCodeSigner},
    {certUsageUserCertImport,        ekuIndexUnknown},
    {certUsageVerifyCA,              ekuIndexUnknown},
    {certUsageProtectedObjectSigner, ekuIndexUnknown},
    {certUsageStatusResponder,       ekuIndexStatusResponder},
    {certUsageAnyCA,                 ekuIndexUnknown},
};

#define CERT_USAGE_EKU_STRING_MAPS_TOTAL       12

/*
 * FUNCTION: cert_NssCertificateUsageToPkixKUAndEKU
 * DESCRIPTION:
 *
 * Converts nss CERTCertificateUsage bit field to pkix key and
 * extended key usages.
 *
 * PARAMETERS:
 *  "cert"
 *      Pointer to CERTCertificate structure of validating cert.
 *  "requiredCertUsages"
 *      Required usage that will be converted to pkix eku and ku. 
 *  "requiredKeyUsage",
 *      Additional key usages impose to cert.
 *  "isCA",
 *      it true, convert usages for cert that is a CA cert.  
 *  "ppkixEKUList"
 *      Returned address of a list of pkix extended key usages.
 *  "ppkixKU"
 *      Returned address of pkix required key usages bit field. 
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_NssCertificateUsageToPkixKUAndEKU(
    CERTCertificate *cert,
    SECCertUsage     requiredCertUsage,
    PRUint32         requiredKeyUsages,
    PRBool           isCA,
    PKIX_List      **ppkixEKUList,
    PKIX_UInt32     *ppkixKU,
    void            *plContext)
{
    PKIX_List           *ekuOidsList = NULL;
    PKIX_PL_OID         *ekuOid = NULL;
    int                  i = 0;
    int                  ekuIndex = ekuIndexUnknown;

    PKIX_ENTER(CERTVFYPKIX, "cert_NssCertificateUsageToPkixEku");
    PKIX_NULLCHECK_TWO(ppkixEKUList, ppkixKU);
    
    PKIX_CHECK(
        PKIX_List_Create(&ekuOidsList, plContext),
        PKIX_LISTCREATEFAILED);

    for (;i < CERT_USAGE_EKU_STRING_MAPS_TOTAL;i++) {
        const SECCertUsageToEku *usageToEkuElem =
            &certUsageEkuStringMap[i];
        if (usageToEkuElem->certUsage == requiredCertUsage) {
            ekuIndex = usageToEkuElem->ekuStringIndex;
            break;
        }
    }
    if (ekuIndex != ekuIndexUnknown) {
        PRUint32             reqKeyUsage = 0;
        PRUint32             reqCertType = 0;

        CERT_KeyUsageAndTypeForCertUsage(requiredCertUsage, isCA,
                                         &reqKeyUsage,
                                         &reqCertType);
        
        requiredKeyUsages |= reqKeyUsage;
        
        PKIX_CHECK(
            PKIX_PL_OID_Create(ekuOidStrings[ekuIndex], &ekuOid,
                               plContext),
            PKIX_OIDCREATEFAILED);
        
        PKIX_CHECK(
            PKIX_List_AppendItem(ekuOidsList, (PKIX_PL_Object *)ekuOid,
                                 plContext),
            PKIX_LISTAPPENDITEMFAILED);
        
        PKIX_DECREF(ekuOid);
    }

    PKIX_CHECK(
        cert_NssKeyUsagesToPkix(requiredKeyUsages, ppkixKU, plContext),
        PKIX_NSSCERTIFICATEUSAGETOPKIXKUANDEKUFAILED);

    *ppkixEKUList = ekuOidsList;
    ekuOidsList = NULL;

cleanup:
    
    PKIX_DECREF(ekuOid);
    PKIX_DECREF(ekuOidsList);

    PKIX_RETURN(CERTVFYPKIX);
}


/*
 * FUNCTION: cert_ProcessingParamsSetKuAndEku
 * DESCRIPTION:
 *
 * Converts cert usage to pkix KU and EKU types and sets
 * converted data into PKIX_ProcessingParams object. It also sets
 * proper cert usage into nsscontext object.
 *
 * PARAMETERS:
 *  "procParams"
 *      Pointer to PKIX_ProcessingParams used during validation.
 *  "requiredCertUsage"
 *      Required certificate usages the certificate and chain is built and
 *      validated for.
 *  "requiredKeyUsage"
 *      Request additional key usages the certificate should be validated for.
 *  "isCA"
 *      Should the cert be verifyed as CA cert for the usages.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_ProcessingParamsSetKuAndEku(
    PKIX_ProcessingParams *procParams,
    CERTCertificate       *cert,
    PRBool                 isCA,
    SECCertUsage           requiredCertUsage,
    PRUint32               requiredKeyUsages,
    void                  *plContext)
{
    PKIX_List             *extKeyUsage = NULL;
    PKIX_CertSelector     *certSelector = NULL;
    PKIX_ComCertSelParams *certSelParams = NULL;
    PKIX_PL_NssContext    *nssContext = (PKIX_PL_NssContext*)plContext;
    PKIX_UInt32            keyUsage = 0;
 
    PKIX_ENTER(CERTVFYPKIX, "cert_ProcessingParamsSetKuAndEku");
    PKIX_NULLCHECK_TWO(procParams, nssContext);
    
    PKIX_CHECK(
        pkix_pl_NssContext_SetCertUsage(
	    ((SECCertificateUsage)1) << requiredCertUsage, nssContext),
	    PKIX_NSSCONTEXTSETCERTUSAGEFAILED);

    PKIX_CHECK(
        cert_NssCertificateUsageToPkixKUAndEKU(cert, requiredCertUsage,
                                               requiredKeyUsages, isCA, 
                                               &extKeyUsage, &keyUsage,
                                               plContext),
        PKIX_CANNOTCONVERTCERTUSAGETOPKIXKEYANDEKUSAGES);

    PKIX_CHECK(
        PKIX_ProcessingParams_GetTargetCertConstraints(procParams,
                                                       &certSelector, plContext),
        PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);

    PKIX_CHECK(
        PKIX_CertSelector_GetCommonCertSelectorParams(certSelector,
                                                      &certSelParams, plContext),
        PKIX_CERTSELECTORGETCOMMONCERTSELECTORPARAMSFAILED);
    

    PKIX_CHECK(
        PKIX_ComCertSelParams_SetKeyUsage(certSelParams, keyUsage,
                                          plContext),
        PKIX_COMCERTSELPARAMSSETKEYUSAGEFAILED);

    PKIX_CHECK(
        PKIX_ComCertSelParams_SetExtendedKeyUsage(certSelParams,
                                                  extKeyUsage,
                                                  plContext),
        PKIX_COMCERTSELPARAMSSETEXTKEYUSAGEFAILED);

    PKIX_CHECK(
        PKIX_PL_EkuChecker_Create(procParams, plContext),
        PKIX_EKUCHECKERINITIALIZEFAILED);

cleanup:
    PKIX_DECREF(extKeyUsage);
    PKIX_DECREF(certSelector);
    PKIX_DECREF(certSelParams);

    PKIX_RETURN(CERTVFYPKIX);
}

/*
 * Unused parameters: 
 *
 *  CERTCertList *initialChain,
 *  CERTCertStores certStores,
 *  CERTCertRevCheckers certRevCheckers,
 *  CERTCertChainCheckers certChainCheckers,
 *  SECItem *initPolicies,
 *  PRBool policyQualifierRejected,
 *  PRBool anyPolicyInhibited,
 *  PRBool reqExplicitPolicy,
 *  PRBool policyMappingInhibited,
 *  PKIX_CertSelector certConstraints,
 */

/*
 * FUNCTION: cert_CreatePkixProcessingParams
 * DESCRIPTION:
 *
 * Creates and fills in PKIX_ProcessingParams structure to be used
 * for certificate chain building.
 *
 * PARAMETERS:
 *  "cert"
 *      Pointer to the CERTCertificate: the leaf certificate of a chain.
 *  "time"
 *      Validity time.
 *  "wincx"
 *      Nss db password token.
 *  "useArena"
 *      Flags to use arena for data allocation during chain building process.
 *  "pprocParams"
 *      Address to return created processing parameters.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_CreatePkixProcessingParams(
    CERTCertificate        *cert,
    PRBool                  checkSig, /* not used yet. See bug 391476 */
    PRTime                  time,
    void                   *wincx,
    PRBool                  useArena,
#ifdef DEBUG_volkov
    PRBool                  checkAllCertsOCSP,
#endif
    PKIX_ProcessingParams **pprocParams,
    void                  **pplContext)
{
    PKIX_List             *anchors = NULL;
    PKIX_PL_Cert          *targetCert = NULL;
    PKIX_PL_Date          *date = NULL;
    PKIX_ProcessingParams *procParams = NULL;
    PKIX_CertSelector     *certSelector = NULL;
    PKIX_ComCertSelParams *certSelParams = NULL;
    PKIX_CertStore        *certStore = NULL;
    PKIX_List             *certStores = NULL;
#ifdef DEBUG_volkov
    PKIX_RevocationChecker *ocspChecker = NULL;
#endif
    void                  *plContext = NULL;
    
    PKIX_ENTER(CERTVFYPKIX, "cert_CreatePkixProcessingParams");
    PKIX_NULLCHECK_TWO(cert, pprocParams);
 
    PKIX_CHECK(
        PKIX_PL_NssContext_Create(0, useArena, wincx, &plContext),
        PKIX_NSSCONTEXTCREATEFAILED);

    *pplContext = plContext;

#ifdef PKIX_NOTDEF 
    /* Functions should be implemented in patch for 390532 */
    PKIX_CHECK(
        pkix_pl_NssContext_SetCertSignatureCheck(checkSig,
                                                 (PKIX_PL_NssContext*)plContext),
        PKIX_NSSCONTEXTSETCERTSIGNCHECKFAILED);

#endif /* PKIX_NOTDEF */

    PKIX_CHECK(
        PKIX_ProcessingParams_Create(&procParams, plContext),
        PKIX_PROCESSINGPARAMSCREATEFAILED);
    
    PKIX_CHECK(
        PKIX_ComCertSelParams_Create(&certSelParams, plContext),
        PKIX_COMCERTSELPARAMSCREATEFAILED);
    
    PKIX_CHECK(
        PKIX_PL_Cert_CreateFromCERTCertificate(cert, &targetCert, plContext),
        PKIX_CERTCREATEWITHNSSCERTFAILED);

    PKIX_CHECK(
        PKIX_ComCertSelParams_SetCertificate(certSelParams,
                                             targetCert, plContext),
        PKIX_COMCERTSELPARAMSSETCERTIFICATEFAILED);
    
    PKIX_CHECK(
        PKIX_CertSelector_Create(NULL, NULL, &certSelector, plContext),
        PKIX_COULDNOTCREATECERTSELECTOROBJECT);
    
    PKIX_CHECK(
        PKIX_CertSelector_SetCommonCertSelectorParams(certSelector,
                                                      certSelParams, plContext),
        PKIX_CERTSELECTORSETCOMMONCERTSELECTORPARAMSFAILED);
    
    PKIX_CHECK(
        PKIX_ProcessingParams_SetTargetCertConstraints(procParams,
                                                       certSelector, plContext),
        PKIX_PROCESSINGPARAMSSETTARGETCERTCONSTRAINTSFAILED);

    PKIX_CHECK(
        PKIX_PL_Pk11CertStore_Create(&certStore, plContext),
        PKIX_PK11CERTSTORECREATEFAILED);
    
    PKIX_CHECK(
        PKIX_List_Create(&certStores, plContext),
        PKIX_UNABLETOCREATELIST);
    
    PKIX_CHECK(
        PKIX_List_AppendItem(certStores, (PKIX_PL_Object *)certStore,
                             plContext),
        PKIX_LISTAPPENDITEMFAILED);

    PKIX_CHECK(
        PKIX_ProcessingParams_SetCertStores(procParams, certStores,
                                            plContext),
        PKIX_PROCESSINGPARAMSADDCERTSTOREFAILED);

    PKIX_CHECK(
        PKIX_PL_Date_CreateFromPRTime(time, &date, plContext),
        PKIX_DATECREATEFROMPRTIMEFAILED);

    PKIX_CHECK(
        PKIX_ProcessingParams_SetDate(procParams, date, plContext),
        PKIX_PROCESSINGPARAMSSETDATEFAILED);
    
    PKIX_CHECK(
        PKIX_ProcessingParams_SetNISTRevocationPolicyEnabled(procParams,
                                                             PKIX_FALSE,
                                                             plContext),
        PKIX_PROCESSINGPARAMSSETNISTREVOCATIONENABLEDFAILED);

#ifdef DEBUG_volkov1
    /* Enables ocsp rev checking of the chain cert through pkix OCSP
     * implementation. */
    if (checkAllCertsOCSP) {
        PKIX_CHECK(
            PKIX_OcspChecker_Initialize(date, NULL, NULL, 
                                        &ocspChecker, plContext),
            PKIX_PROCESSINGPARAMSSETDATEFAILED);
        
        PKIX_CHECK(
            PKIX_ProcessingParams_AddRevocationChecker(procParams,
                                                       ocspChecker, plContext),
            PKIX_PROCESSINGPARAMSSETDATEFAILED);
    }
#endif

    PKIX_CHECK(
        PKIX_ProcessingParams_SetAnyPolicyInhibited(procParams, PR_FALSE,
                                                    plContext),
        PKIX_PROCESSINGPARAMSSETANYPOLICYINHIBITED);

    PKIX_CHECK(
        PKIX_ProcessingParams_SetExplicitPolicyRequired(procParams, PR_FALSE,
                                                       plContext),
        PKIX_PROCESSINGPARAMSSETEXPLICITPOLICYREQUIRED);

    PKIX_CHECK(
        PKIX_ProcessingParams_SetPolicyMappingInhibited(procParams, PR_FALSE,
                                                        plContext),
        PKIX_PROCESSINGPARAMSSETPOLICYMAPPINGINHIBITED);
 
    *pprocParams = procParams;
    procParams = NULL;

cleanup:
    PKIX_DECREF(anchors);
    PKIX_DECREF(targetCert);
    PKIX_DECREF(date);
    PKIX_DECREF(certSelector);
    PKIX_DECREF(certSelParams);
    PKIX_DECREF(certStore);
    PKIX_DECREF(certStores);
    PKIX_DECREF(procParams);
#ifdef DEBUG_volkov    
    PKIX_DECREF(ocspChecker);
#endif

    PKIX_RETURN(CERTVFYPKIX);
}

/*
 * FUNCTION: cert_PkixToNssCertsChain
 * DESCRIPTION:
 *
 * Converts pkix cert list into nss cert list.
 * 
 * PARAMETERS:
 *  "pkixCertChain"
 *      Pkix certificate list.     
 *  "pvalidChain"
 *      An address of returned nss certificate list.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_PkixToNssCertsChain(
    PKIX_List *pkixCertChain, 
    CERTCertList **pvalidChain, 
    void *plContext)
{
    PRArenaPool     *arena = NULL;
    CERTCertificate *nssCert = NULL;
    CERTCertList    *validChain = NULL;
    PKIX_PL_Object  *certItem = NULL;
    PKIX_UInt32      length = 0;
    PKIX_UInt32      i = 0;

    PKIX_ENTER(CERTVFYPKIX, "cert_PkixToNssCertsChain");
    PKIX_NULLCHECK_ONE(pvalidChain);

    if (pkixCertChain == NULL) {
        goto cleanup;
    }
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
        PKIX_ERROR(PKIX_OUTOFMEMORY);
    }
    validChain = (CERTCertList*)PORT_ArenaZAlloc(arena, sizeof(CERTCertList));
    if (validChain == NULL) {
        PKIX_ERROR(PKIX_PORTARENAALLOCFAILED);
    }
    PR_INIT_CLIST(&validChain->list);
    validChain->arena = arena;
    arena = NULL;

    PKIX_CHECK(
        PKIX_List_GetLength(pkixCertChain, &length, plContext),
        PKIX_LISTGETLENGTHFAILED);

    for (i = 0; i < length; i++){
        CERTCertListNode *node = NULL;

        PKIX_CHECK(
            PKIX_List_GetItem(pkixCertChain, i, &certItem, plContext),
            PKIX_LISTGETITEMFAILED);
        
        PKIX_CHECK(
            PKIX_PL_Cert_GetCERTCertificate((PKIX_PL_Cert*)certItem, &nssCert,
                                    plContext),
            PKIX_CERTGETCERTCERTIFICATEFAILED);
        
        node =
            (CERTCertListNode *)PORT_ArenaZAlloc(validChain->arena,
                                                 sizeof(CERTCertListNode));
        if ( node == NULL ) {
            PKIX_ERROR(PKIX_PORTARENAALLOCFAILED);
        }

        PR_INSERT_BEFORE(&node->links, &validChain->list);

        node->cert = nssCert;
        nssCert = NULL;

        PKIX_DECREF(certItem);
    }

    *pvalidChain = validChain;

cleanup:
    if (PKIX_ERROR_RECEIVED){
        if (validChain) {
            CERT_DestroyCertList(validChain);
        } else if (arena) {
            PORT_FreeArena(arena, PR_FALSE);
        }
        if (nssCert) {
            CERT_DestroyCertificate(nssCert);
        }
    }
    PKIX_DECREF(certItem);

    PKIX_RETURN(CERTVFYPKIX);
}


/*
 * FUNCTION: cert_BuildAndValidateChain
 * DESCRIPTION:
 *
 * The function builds and validates a cert chain based on certificate
 * selection criterias from procParams. This function call PKIX_BuildChain
 * to accomplish chain building. If PKIX_BuildChain returns with incomplete
 * IO, the function waits with PR_Poll until the blocking IO is finished and
 * return control back to PKIX_BuildChain.
 *
 * PARAMETERS:
 *  "procParams"
 *      Processing parameters to be used during chain building.
 *  "pResult"
 *      Returned build result.
 *  "pVerifyNode"
 *      Returned pointed to verify node structure: the tree-like structure
 *      that reports points of chain building failures.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_BuildAndValidateChain(
    PKIX_ProcessingParams *procParams,
    PKIX_BuildResult **pResult,
    PKIX_VerifyNode **pVerifyNode,
    void *plContext)
{
    PKIX_BuildResult *result = NULL;
    PKIX_VerifyNode  *verifyNode = NULL;
    void             *nbioContext = NULL;
    void             *state = NULL;
    
    PKIX_ENTER(CERTVFYPKIX, "cert_BuildAndVerifyChain");
    PKIX_NULLCHECK_TWO(procParams, pResult);
 
    do {
        if (nbioContext && state) {
            /* PKIX-XXX: need to test functionality of NBIO handling in libPkix.
             * See bug 391180 */
            PRInt32 filesReady = 0;
            PRPollDesc *pollDesc = (PRPollDesc*)nbioContext;
            filesReady = PR_Poll(pollDesc, 1, PR_INTERVAL_NO_TIMEOUT);
            if (filesReady <= 0) {
                PKIX_ERROR(PKIX_PRPOLLRETBADFILENUM);
            }
        }

        PKIX_CHECK(
            PKIX_BuildChain(procParams, &nbioContext, &state,
                            &result, &verifyNode, plContext),
            PKIX_UNABLETOBUILDCHAIN);
        
    } while (nbioContext && state);

    *pResult = result;

cleanup:
    if (pVerifyNode) {
        *pVerifyNode = verifyNode;
    }

    PKIX_RETURN(CERTVFYPKIX);
}


/*
 * FUNCTION: cert_PkixErrorToNssCode
 * DESCRIPTION:
 *
 * Converts pkix error(PKIX_Error) structure to PR error codes.
 *
 * PKIX-XXX to be implemented. See 391183.
 *
 * PARAMETERS:
 *  "error"
 *      Pkix error that will be converted.
 *  "nssCode"
 *      Corresponding nss error code.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
cert_PkixErrorToNssCode(
    PKIX_Error *error,
    SECErrorCodes *pNssErr,
    void *plContext)
{
    int errLevel = 0;
    PKIX_UInt32 nssErr = 0;
    PKIX_Error *errPtr = error;

    PKIX_ENTER(CERTVFYPKIX, "cert_PkixErrorToNssCode");
    PKIX_NULLCHECK_TWO(error, pNssErr);
    
    /* Loop until we find at least one error with non-null
     * plErr code, that is going to be nss error code. */
    while (errPtr) {
        if (errPtr->plErr && !nssErr) {
            nssErr = errPtr->plErr;
            if (!pkixLog) break;
        }
        if (pkixLog) {
            PR_LOG(pkixLog, 1, ("Error at level %d: %s\n", errLevel,
                                PKIX_ErrorText[errPtr->errCode]));
        }
        errPtr = errPtr->cause;
        errLevel += 1; 
    }
    PORT_Assert(nssErr);
    if (!nssErr) {
        *pNssErr = SEC_ERROR_LIBPKIX_INTERNAL;
    } else {
        *pNssErr = nssErr;
    }

    PKIX_RETURN(CERTVFYPKIX);
}

/*
 * FUNCTION: cert_GetLogFromVerifyNode
 * DESCRIPTION:
 *
 * Recursive function that converts verify node tree-like set of structures
 * to CERTVerifyLog.
 *
 * PARAMETERS:
 *  "log"
 *      Pointed to already allocated CERTVerifyLog structure. 
 *  "node"
 *      A node of PKIX_VerifyNode tree.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
cert_GetLogFromVerifyNode(
    CERTVerifyLog *log,
    PKIX_VerifyNode *node,
    void *plContext)
{
    PKIX_List       *children = NULL;
    PKIX_VerifyNode *childNode = NULL;

    PKIX_ENTER(CERTVFYPKIX, "cert_GetLogFromVerifyNode");

    children = node->children;

    if (children == NULL) {
        PKIX_ERRORCODE errCode = PKIX_ANCHORDIDNOTCHAINTOCERT;
        if (node->error && node->error->errCode != errCode) {
#ifdef DEBUG_volkov
            char *string = pkix_Error2ASCII(node->error, plContext);
            fprintf(stderr, "Branch search finished with error: \t%s\n", string);
            PKIX_PL_Free(string, NULL);
#endif
            if (log != NULL) {
                SECErrorCodes nssErrorCode = 0;
                CERTCertificate *cert = NULL;

                cert = node->verifyCert->nssCert;

                PKIX_CHECK(
                    cert_PkixErrorToNssCode(node->error, &nssErrorCode,
                                            plContext),
                    PKIX_GETPKIXERRORCODEFAILED);
                
                cert_AddToVerifyLog(log, cert, nssErrorCode, node->depth, NULL);
            }
        }
        PKIX_RETURN(CERTVFYPKIX);
    } else {
        PRUint32      i = 0;
        PKIX_UInt32   length = 0;

        PKIX_CHECK(
            PKIX_List_GetLength(children, &length, plContext),
            PKIX_LISTGETLENGTHFAILED);
        
        for (i = 0; i < length; i++){

            PKIX_CHECK(
                PKIX_List_GetItem(children, i, (PKIX_PL_Object**)&childNode,
                                  plContext),
                PKIX_LISTGETITEMFAILED);
            
            PKIX_CHECK(
                cert_GetLogFromVerifyNode(log, childNode, plContext),
                PKIX_ERRORINRECURSIVEEQUALSCALL);

            PKIX_DECREF(childNode);
        }
    }

cleanup:
    PKIX_DECREF(childNode);

    PKIX_RETURN(CERTVFYPKIX);
}

/*
 * FUNCTION: cert_GetBuildResults
 * DESCRIPTION:
 *
 * Converts pkix build results to nss results. This function is called
 * regardless of build result.
 *
 * If it called after chain was successfully constructed, then it will
 * convert:
 *   * pkix cert list that represent the chain to nss cert list
 *   * trusted root the chain was anchored to nss certificate.
 *
 * In case of failure it will convert:
 *   * pkix error to PR error code(will set it with PORT_SetError)
 *   * pkix validation log to nss CERTVerifyLog
 *   
 * PARAMETERS:
 *  "buildResult"
 *      Build results returned by PKIX_BuildChain.
 *  "verifyNode"
 *      Tree-like structure of chain building/validation failures
 *      returned by PKIX_BuildChain. Ignored in case of success.
 *  "error"
 *      Final error returned by PKIX_BuildChain. Should be NULL in
 *      case of success.
 *  "log"
 *      Address of pre-allocated(if not NULL) CERTVerifyLog structure.
 *  "ptrustedRoot"
 *      Address of returned trusted root the chain was anchored to.
 *  "pvalidChain"
 *      Address of returned valid chain.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Cert Verify Error if the function fails in an unrecoverable way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
cert_GetBuildResults(
    PKIX_BuildResult *buildResult,
    PKIX_VerifyNode  *verifyNode,
    PKIX_Error       *error,
    CERTVerifyLog    *log,
    CERTCertificate **ptrustedRoot,
    CERTCertList    **pvalidChain,
    void             *plContext)
{
    PKIX_ValidateResult *validResult = NULL;
    CERTCertList        *validChain = NULL;
    CERTCertificate     *trustedRoot = NULL;
    PKIX_TrustAnchor    *trustAnchor = NULL;
    PKIX_PL_Cert        *trustedCert = NULL;
    PKIX_List           *pkixCertChain = NULL;
#ifdef DEBUG_volkov
    PKIX_Error          *tmpPkixError = NULL;
#endif /* DEBUG */
            
    PKIX_ENTER(CERTVFYPKIX, "cert_GetBuildResults");
    if (buildResult == NULL && error == NULL) {
        PKIX_ERROR(PKIX_NULLARGUMENT);
    }

    if (error) {
        SECErrorCodes nssErrorCode = 0;
#ifdef DEBUG_volkov        
        char *temp = pkix_Error2ASCII(error, plContext);
        fprintf(stderr, "BUILD ERROR:\n%s\n", temp);
        PKIX_PL_Free(temp, NULL);
#endif /* DEBUG */
        cert_PkixErrorToNssCode(error, &nssErrorCode, plContext);
        PORT_SetError(nssErrorCode);
        
        if (verifyNode) {
            PKIX_Error *tmpError =
                cert_GetLogFromVerifyNode(log, verifyNode, plContext);
            if (tmpError) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)tmpError, plContext);
            }
        }
        goto cleanup;
    }

    if (pvalidChain) {
        PKIX_CHECK(
            PKIX_BuildResult_GetCertChain(buildResult, &pkixCertChain,
                                          plContext),
            PKIX_BUILDRESULTGETCERTCHAINFAILED);

#ifdef DEBUG_volkov
        tmpPkixError = cert_PrintCertChain(pkixCertChain, plContext);
        if (tmpPkixError) {
            PKIX_PL_Object_DecRef((PKIX_PL_Object*)tmpPkixError, plContext);
        }
#endif        

        PKIX_CHECK(
            cert_PkixToNssCertsChain(pkixCertChain, &validChain, plContext),
            PKIX_CERTCHAINTONSSCHAINFAILED);
    }

    if (ptrustedRoot) {
        PKIX_CHECK(
            PKIX_BuildResult_GetValidateResult(buildResult, &validResult,
                                               plContext),
            PKIX_BUILDRESULTGETVALIDATERESULTFAILED);

        PKIX_CHECK(
            PKIX_ValidateResult_GetTrustAnchor(validResult, &trustAnchor,
                                               plContext),
            PKIX_VALIDATERESULTGETTRUSTANCHORFAILED);

        PKIX_CHECK(
            PKIX_TrustAnchor_GetTrustedCert(trustAnchor, &trustedCert,
                                            plContext),
            PKIX_TRUSTANCHORGETTRUSTEDCERTFAILED);

#ifdef DEBUG_volkov
        if (pvalidChain == NULL) {
            cert_PrintCert(trustedCert, plContext);
        }
#endif        

       PKIX_CHECK(
            PKIX_PL_Cert_GetCERTCertificate(trustedCert, &trustedRoot,
                                            plContext),
            PKIX_CERTGETCERTCERTIFICATEFAILED);
    }
 
    PORT_Assert(!PKIX_ERROR_RECEIVED);

    if (trustedRoot) {
        *ptrustedRoot = trustedRoot;
    }
    if (validChain) {
        *pvalidChain = validChain;
    }

cleanup:
    if (PKIX_ERROR_RECEIVED) {
        if (trustedRoot) {
            CERT_DestroyCertificate(trustedRoot);
        }
        if (validChain) {
            CERT_DestroyCertList(validChain);
        }
    }
    PKIX_DECREF(trustAnchor);
    PKIX_DECREF(trustedCert);
    PKIX_DECREF(pkixCertChain);
    PKIX_DECREF(validResult);
    PKIX_DECREF(error);
    PKIX_DECREF(verifyNode);
    PKIX_DECREF(buildResult);
    
    PKIX_RETURN(CERTVFYPKIX);
}

/*
 * FUNCTION: cert_VerifyCertChainPkix
 * DESCRIPTION:
 *
 * The main wrapper function that is called from CERT_VerifyCert and
 * CERT_VerifyCACertForUsage functions to validate cert with libpkix.
 *
 * PARAMETERS:
 *  "cert"
 *      Leaf certificate of a chain we want to build.
 *  "checkSig"
 *      Certificate signatures will not be verified if this
 *      flag is set to PR_FALSE.
 *  "requiredUsage"
 *      Required usage for certificate and chain.
 *  "time"
 *      Validity time.
 *  "wincx"
 *      Nss database password token.
 *  "log"
 *      Address of already allocated CERTVerifyLog structure. Not
 *      used if NULL;
 *  "pSigerror"
 *      Address of PRBool. If not NULL, returns true is cert chain
 *      was invalidated because of bad certificate signature.
 *  "pRevoked"
 *      Address of PRBool. If not NULL, returns true is cert chain
 *      was invalidated because a revoked certificate was found in
 *      the chain.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  SECFailure is chain building process has failed. SECSuccess otherwise.
 */
SECStatus
cert_VerifyCertChainPkix(
    CERTCertificate *cert,
    PRBool           checkSig,
    SECCertUsage     requiredUsage,
    PRUint64         time,
    void            *wincx,
    CERTVerifyLog   *log,
    PRBool          *pSigerror,
    PRBool          *pRevoked)
{
    PKIX_ProcessingParams *procParams = NULL;
    PKIX_BuildResult      *result = NULL;
    PKIX_VerifyNode       *verifyNode = NULL;
    PKIX_Error            *error = NULL;

    SECStatus              rv = SECFailure;
    void                  *plContext = NULL;
#ifdef DEBUG_volkov
    CERTCertificate       *trustedRoot = NULL;
    CERTCertList          *validChain = NULL;
#endif /* DEBUG */

#ifdef PKIX_OBJECT_LEAK_TEST
    int  leakedObjNum = 0;
    int  memLeakLoopCount = 0;
    int  objCountTable[PKIX_NUMTYPES]; 
    int  fnInvLocalCount = 0;

    fnStackNameArr[0] = "cert_VerifyCertChainPkix";
    fnStackInvCountArr[0] = 0;
    PKIX_Boolean abortOnLeak = 
        (PR_GetEnv("PKIX_OBJECT_LEAK_TEST_ABORT_ON_LEAK") == NULL) ?
                                                   PKIX_FALSE : PKIX_TRUE;
    runningLeakTest = PKIX_TRUE;

    /* Prevent multi-threaded run of object leak test */
    fnInvLocalCount = PR_AtomicIncrement(&parallelFnInvocationCount);
    PORT_Assert(fnInvLocalCount == 1);

do {
    rv = SECFailure;
    plContext = NULL;
    procParams = NULL;
    result = NULL;
    verifyNode = NULL;
    error = NULL;
#ifdef DEBUG_volkov
    trustedRoot = NULL;
    validChain = NULL;
#endif /* DEBUG */
    errorGenerated = PKIX_FALSE;
    stackPosition = 0;

    if (leakedObjNum) {
        pkix_pl_lifecycle_ObjectTableUpdate(objCountTable); 
    }

    PR_LOG(pkixLog, 1, ("Memory leak test: Loop %d\n", memLeakLoopCount));
#endif /* PKIX_OBJECT_LEAK_TEST */

    error =
        cert_CreatePkixProcessingParams(cert, checkSig, time, wincx,
                                        PR_FALSE/*use arena*/,
#ifdef DEBUG_volkov
                                        /* If in DEBUG_volkov, then enable OCSP
                                         * check for all certs in the chain
                                         * using libpkix ocsp code.
                                         * (except for certUsageStatusResponder). */
                                        requiredUsage != certUsageStatusResponder,
#endif
                                        &procParams, &plContext);
    if (error) {
        goto cleanup;
    }

    error =
        cert_ProcessingParamsSetKuAndEku(procParams, cert, PR_TRUE,
                                         requiredUsage, 0, plContext);
    if (error) {
        goto cleanup;
    }

    error = 
        cert_BuildAndValidateChain(procParams, &result, &verifyNode, plContext);
    if (error) {
        goto cleanup;
    }
    
    if (pRevoked) {
        /* Currently always PR_FALSE. Will be fixed as a part of 394077 */
        *pRevoked = PR_FALSE;
    }
    if (pSigerror) {
        /* Currently always PR_FALSE. Will be fixed as a part of 394077 */
        *pSigerror = PR_FALSE;
    }
    rv = SECSuccess;

cleanup:
    error = cert_GetBuildResults(result, verifyNode, error, log,
#ifdef DEBUG_volkov                                 
                                 &trustedRoot, &validChain,
#else
                                 NULL, NULL,
#endif /* DEBUG */
                                 plContext);
    if (error) {
#ifdef DEBUG_volkov        
        char *temp = pkix_Error2ASCII(error, plContext);
        fprintf(stderr, "GET BUILD RES ERRORS:\n%s\n", temp);
        PKIX_PL_Free(temp, NULL);
#endif /* DEBUG */
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)error, plContext);
    }
#ifdef DEBUG_volkov
    if (trustedRoot) {
        CERT_DestroyCertificate(trustedRoot);
    }
    if (validChain) {
        CERT_DestroyCertList(validChain);
    }
#endif /* DEBUG */
    if (procParams) {
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)procParams, plContext);
    }
    if (plContext) {
        PKIX_PL_NssContext_Destroy(plContext);
    }

#ifdef PKIX_OBJECT_LEAK_TEST
    leakedObjNum =
        pkix_pl_lifecycle_ObjectLeakCheck(leakedObjNum ? objCountTable : NULL);
    
    if (abortOnLeak) {
        PORT_Assert(leakedObjNum == 0);
    }

} while (errorGenerated);

    runningLeakTest = PKIX_FALSE; 
    PR_AtomicDecrement(&parallelFnInvocationCount);
#endif /* PKIX_OBJECT_LEAK_TEST */

    return rv;
}

PKIX_CertSelector *
cert_GetTargetCertConstraints(CERTCertificate *target, void *plContext) 
{
    PKIX_ComCertSelParams *certSelParams = NULL;
    PKIX_CertSelector *certSelector = NULL;
    PKIX_CertSelector *r= NULL;
    PKIX_PL_Cert *eeCert = NULL;
    PKIX_Error *error = NULL;

    error = PKIX_PL_Cert_CreateFromCERTCertificate(target, &eeCert, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_CertSelector_Create(NULL, NULL, &certSelector, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_ComCertSelParams_Create(&certSelParams, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_ComCertSelParams_SetCertificate(
                                certSelParams, eeCert, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_CertSelector_SetCommonCertSelectorParams
        (certSelector, certSelParams, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_PL_Object_IncRef((PKIX_PL_Object *)certSelector, plContext);
    if (error == NULL) r = certSelector;

cleanup:
    if (certSelParams != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)certSelParams, plContext);

    if (eeCert != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)eeCert, plContext);

    if (certSelector != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)certSelector, plContext);

    if (error != NULL) {
	SECErrorCodes nssErr;

	cert_PkixErrorToNssCode(error, &nssErr, plContext);
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)error, plContext);
	PORT_SetError(nssErr);
    }

    return r;
}

static PKIX_List *
cert_GetCertStores(void *plContext)
{
    PKIX_CertStore *certStore = NULL;
    PKIX_List *certStores = NULL;
    PKIX_List *r = NULL;
    PKIX_Error *error = NULL;

    error = PKIX_PL_Pk11CertStore_Create(&certStore, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_List_Create(&certStores, plContext);
    if (error != NULL)  goto cleanup;

    error = PKIX_List_AppendItem( certStores, 
                          (PKIX_PL_Object *)certStore, plContext);
    if (error != NULL)  goto cleanup;

    error = PKIX_PL_Object_IncRef((PKIX_PL_Object *)certStores, plContext);
    if (error == NULL) r = certStores;

cleanup:
    if (certStores != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)certStores, plContext);

    if (certStore != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)certStore, plContext);

    if (error != NULL) {
	SECErrorCodes nssErr;

	cert_PkixErrorToNssCode(error, &nssErr, plContext);
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)error, plContext);
	PORT_SetError(nssErr);
    }

    return r;
}

/* XXX
 *  There is no NSS SECItem -> PKIX OID
 *  conversion function. For now, I go via the ascii
 *  representation 
 *  this should be in PKIX_PL_*
 */

PKIX_PL_OID *
CERT_PKIXOIDFromNSSOid(SECOidTag tag, void*plContext)
{
    char *oidstring = NULL;
    char *oidstring_adj = NULL;
    PKIX_PL_OID *policyOID = NULL;
    SECOidData *data;

    data =  SECOID_FindOIDByTag(tag);
    if (data != NULL) {
        oidstring = CERT_GetOidString(&data->oid);
        if (oidstring == NULL) {
            goto cleanup;
        }
        oidstring_adj = oidstring;
        if (PORT_Strncmp("OID.",oidstring_adj,4) == 0) {
            oidstring_adj += 4;
        }

        PKIX_PL_OID_Create(oidstring_adj, &policyOID, plContext);
    }
cleanup:
    if (oidstring != NULL) PR_smprintf_free(oidstring);

    return policyOID;
}

struct fake_PKIX_PL_CertStruct {
        CERTCertificate *nssCert;
};

/* This needs to be part of the PKIX_PL_* */
/* This definitely needs to go away, and be replaced with
   a real accessor function in PKIX */
CERTCertificate *
cert_NSSCertFromPKIXCert(const PKIX_PL_Cert *pkix_cert, void *plContext)
{
    struct fake_PKIX_PL_CertStruct *fcert = NULL;

    fcert = (struct fake_PKIX_PL_CertStruct*)pkix_cert;

    return CERT_DupCertificate(fcert->nssCert);
}

PKIX_List *cert_PKIXMakeOIDList(const SECOidTag *oids, int oidCount, void *plContext)
{
    PKIX_List *r = NULL;
    PKIX_List *policyList = NULL;
    PKIX_PL_OID *policyOID = NULL;
    PKIX_Error *error = NULL;
    int i;

    error = PKIX_List_Create(&policyList, plContext);
    if (error != NULL) {
	goto cleanup;
    }

    for (i=0; i<oidCount; i++) {
        policyOID = CERT_PKIXOIDFromNSSOid(oids[i],plContext);
        if (policyOID == NULL) {
            goto cleanup;
        }
        error = PKIX_List_AppendItem(policyList, 
                (PKIX_PL_Object *)policyOID, plContext);
        if (error != NULL) {
            PKIX_PL_Object_DecRef((PKIX_PL_Object *)policyOID, plContext);
            goto cleanup;
        }
    }

    error = PKIX_List_SetImmutable(policyList, plContext);
    if (error != NULL) goto cleanup;

    error = PKIX_PL_Object_IncRef((PKIX_PL_Object *)policyList, plContext);
    if (error == NULL) r = policyList;

cleanup:
    if (policyOID != NULL)  {
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)policyOID, plContext);
    }
    if (policyList != NULL)  {
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)policyList, plContext);
    }
    if (error != NULL)  {
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)error, plContext);
    }

    return r;
}

CERTValOutParam *
cert_pkix_FindOutputParam(CERTValOutParam *params, const CERTValParamOutType t)
{
    CERTValOutParam *i;
    if (params == NULL) {
        return NULL;
    }
    for (i = params; i->type != cert_po_end; i++) {
        if (i->type == t) {
             return i;
        }
    }
    return NULL;
}

SECStatus
cert_pkixSetParam(PKIX_ProcessingParams *procParams, 
  const CERTValInParam *param, void *plContext)
{
    PKIX_Error * error = NULL;
    SECStatus r=SECSuccess;
    PKIX_PL_Date *date = NULL;
    PKIX_List *policyOIDList = NULL;
    PKIX_RevocationChecker *ocspChecker = NULL;
    PKIX_List *certListPkix = NULL;
    const CERTRevocationFlags *flags;
    SECErrorCodes errCode = SEC_ERROR_INVALID_ARGS;
    const CERTCertList *certList = NULL;
    CERTCertListNode *node;
    PKIX_PL_Cert *certPkix = NULL;
    PKIX_TrustAnchor *trustAnchor = NULL;

    /* XXX we need a way to map generic PKIX error to generic NSS errors */

    switch (param->type) {

        case cert_pi_policyOID:

            /* needed? */
            error = PKIX_ProcessingParams_SetExplicitPolicyRequired(
                                procParams, PKIX_TRUE, plContext);

            if (error != NULL) { 
                break;
            }

            policyOIDList = cert_PKIXMakeOIDList(param->value.array.oids,
                                param->value.arraySize,plContext);
	    if (policyOIDList == NULL) {
		r = SECFailure;
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		break;
	    }

            error = PKIX_ProcessingParams_SetInitialPolicies(
                                procParams,policyOIDList,plContext);
            break;

        case cert_pi_date:
            if (param->value.scalar.time == 0) {
                error = PKIX_PL_Date_Create_UTCTime(NULL, &date, plContext);
                if (error != NULL) {
                    errCode = SEC_ERROR_INVALID_TIME;
                    break;
                }
            } else {
                error = pkix_pl_Date_CreateFromPRTime(param->value.scalar.time,
                                                       &date, plContext);
                if (error != NULL) {
                    errCode = SEC_ERROR_INVALID_TIME;
                    break;
                }
            }

            error = PKIX_ProcessingParams_SetDate(procParams, date, plContext);
            if (error != NULL) {
                errCode = SEC_ERROR_INVALID_TIME;
            }
            break;

        case cert_pi_revocationFlags:
        {
            PRBool ocspTurnedOnForLeaf = PR_FALSE;
            PRBool ocspTurnedOnForChain = PR_FALSE;
            PRBool crlTurnedOnForLeaf = PR_FALSE;
            PRBool crlTurnedOnForChain = PR_FALSE;
            PRBool crlHardFailure = PR_FALSE;

            flags = param->value.pointer.revocation;
            if (!flags) {
                PORT_SetError(errCode);
                r = SECFailure;
                break;
            }

            if (
                /* caller did define OCSP leaf behavior */
                (flags->leafTests.number_of_defined_methods >
                    cert_revocation_method_ocsp)
                &&
                /* caller allows OCSP testing for the leaf */
                (flags->leafTests.cert_rev_flags_per_method
                    [cert_revocation_method_ocsp]
                    & CERT_REV_M_TEST_USING_THIS_METHOD)) {
                ocspTurnedOnForLeaf = PR_TRUE;
            }

            if (
                /* caller did define OCSP chain behavior */
                (flags->chainTests.number_of_defined_methods >
                    cert_revocation_method_ocsp)
                &&
                /* caller allows OCSP testing for the chain */
                (flags->chainTests.cert_rev_flags_per_method
                    [cert_revocation_method_ocsp]
                    & CERT_REV_M_TEST_USING_THIS_METHOD)) {
                ocspTurnedOnForChain = PR_TRUE;
            }

            if (
                /* caller did define CRL leaf behavior */
                (flags->leafTests.number_of_defined_methods >
                    cert_revocation_method_crl)
                &&
                /* caller allows CRL testing for the chain */
                (flags->leafTests.cert_rev_flags_per_method
                    [cert_revocation_method_crl]
                    & CERT_REV_M_TEST_USING_THIS_METHOD)) {
                crlTurnedOnForLeaf = PR_TRUE;
            }

            if (
                /* caller did define CRL chain behavior */
                (flags->chainTests.number_of_defined_methods >
                    cert_revocation_method_crl)
                &&
                /* caller allows CRL testing for the chain */
                (flags->chainTests.cert_rev_flags_per_method
                    [cert_revocation_method_crl]
                    & CERT_REV_M_TEST_USING_THIS_METHOD)) {
                crlTurnedOnForChain = PR_TRUE;
            }

            if (
                /* caller did define CRL chain behavior */
                (flags->chainTests.number_of_defined_methods >
                    cert_revocation_method_crl)
                &&
                /* caller requests hard failure on missing (fresh) CRL */
                (flags->chainTests.cert_rev_flags_per_method
                    [cert_revocation_method_crl]
                    & CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO)) {
                /* FIXME: should also consider flag
                 *        CERT_REV_M_SKIP_TEST_ON_MISSING_SOURCE
                 */
                crlHardFailure = PR_TRUE;
            }

            if (!ocspTurnedOnForChain) {
                /* OCSP off either because: 
                 * 1) we didn't  turn ocsp on, or
                 * 2) we are only checking ocsp on the leaf cert only.
                 * The caller needs to handle the leaf case once we add leaf
                 * checking there */

                /* currently OCSP is the only external revocation checker */
                error = PKIX_ProcessingParams_SetRevocationCheckers(procParams,
                        NULL, plContext);
            } else {
                /* FIXME: What should be done if !ocspTurnedOnForLeaf ? */

                /* OCSP is on for the whole chain */
                if (date == NULL) {
                    error = PKIX_ProcessingParams_GetDate
                                        (procParams, &date, plContext );
                    if (error != NULL) {
                        errCode = SEC_ERROR_INVALID_TIME;
                        break;
                    }
                }
                error = PKIX_OcspChecker_Initialize(date, NULL, NULL, 
                                &ocspChecker, plContext);
                if (error != NULL) {
                    break;
                }

                error = PKIX_ProcessingParams_AddRevocationChecker(procParams,
                        ocspChecker, plContext);
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)ocspChecker, plContext);
                ocspChecker=NULL;

                /* FIXME: add support for other revocation flags when underlying
                 * pkix supports it */
            }
            if (error != NULL) {
                break;
            }
            if (!crlTurnedOnForChain) {
                /* CRL checking is off either because: 
                 * 1) we didn't turn crl checking on, or
                 * 2) we are only checking crls on the leaf cert only.
                 * The caller needs to handle the leaf case once we add leaf
                 * checking there */

                /* this function only affects the built-in CRL checker */
                error = PKIX_ProcessingParams_SetRevocationEnabled(procParams,
                        PKIX_FALSE, plContext);
                if (error != NULL) {
                    break;
                }
                /* make sure NIST Revocation Policy is off as well */
                error = PKIX_ProcessingParams_SetNISTRevocationPolicyEnabled
                        (procParams, PKIX_FALSE, plContext);
            } else {
                /* FIXME: What should be done if !crlTurnedOnForLeaf ? */

                /* CRL checking is on for the whole chain */
                error = PKIX_ProcessingParams_SetRevocationEnabled(procParams,
                        PKIX_TRUE, plContext);
                if (error != NULL) {
                    break;
                }
                error = PKIX_ProcessingParams_SetNISTRevocationPolicyEnabled
                    (procParams, 
                     crlHardFailure ? PKIX_TRUE : PKIX_FALSE,
                     plContext);
            }
        }
        break;

        case cert_pi_trustAnchors:
            certList = param->value.pointer.chain;

            error = PKIX_List_Create(&certListPkix, plContext);
            if (error != NULL) {
                break;
            }
            for(node = CERT_LIST_HEAD(certList); !CERT_LIST_END(node, certList);
                node = CERT_LIST_NEXT(node) ) {
                error = PKIX_PL_Cert_CreateFromCERTCertificate(node->cert,
                                                      &certPkix, plContext);
                if (error) {
                    break;
                }
                error = PKIX_TrustAnchor_CreateWithCert(certPkix, &trustAnchor,
                                                        plContext);
                if (error) {
                    break;
                }
                error = PKIX_List_AppendItem(certListPkix,
                                 (PKIX_PL_Object*)trustAnchor, plContext);
                 if (error) {
                    break;
                }
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)trustAnchor, plContext);
                trustAnchor = NULL;
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)certPkix, plContext);
                certPkix = NULL;
            }
            error =
                PKIX_ProcessingParams_SetTrustAnchors(procParams, certListPkix,
                                                      plContext);
            break;

        default:
            PORT_SetError(errCode);
            r = SECFailure;
    }

    if (policyOIDList != NULL)
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)policyOIDList, plContext);

    if (date != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)date, plContext);

    if (ocspChecker != NULL) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)ocspChecker, plContext);

    if (certListPkix) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)certListPkix, plContext);

    if (trustAnchor) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)trustAnchor, plContext);

    if (certPkix) 
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)certPkix, plContext);

    if (error != NULL) {
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)error, plContext);
        PORT_SetError(errCode);
        r = SECFailure;
    }

    return r; 

}

void
cert_pkixDestroyValOutParam(CERTValOutParam *params)
{
    CERTValOutParam *i;

    if (params == NULL) {
        return;
    }
    for (i = params; i->type != cert_po_end; i++) {
        switch (i->type) {
        case cert_po_trustAnchor:
            if (i->value.pointer.cert) {
                CERT_DestroyCertificate(i->value.pointer.cert);
                i->value.pointer.cert = NULL;
            }
            break;

        case cert_po_certList:
            if (i->value.pointer.chain) {
                CERT_DestroyCertList(i->value.pointer.chain);
                i->value.pointer.chain = NULL;
            }
        }
    }
}

static PRUint64 certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy_LeafFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD 
  | CERT_REV_M_FORBID_NETWORK_FETCHING
  | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO,
  /* ocsp */
  CERT_REV_M_TEST_USING_THIS_METHOD
};

static PRUint64 certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy_ChainFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FORBID_NETWORK_FETCHING
  | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO,
  /* ocsp */
  0
};

static CERTRevocationMethodIndex 
certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy_Method_Preference = {
  cert_revocation_method_crl
};

static const CERTRevocationFlags certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy = {
  {
    /* leafTests */
    2,
    certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy_LeafFlags,
    1,
    &certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy_Method_Preference,
    0
  },
  {
    /* chainTests */
    2,
    certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy_ChainFlags,
    0,
    0,
    0
  }
};

extern const CERTRevocationFlags*
CERT_GetClassicOCSPEnabledSoftFailurePolicy()
{
    return &certRev_NSS_3_11_Ocsp_Enabled_Soft_Policy;
}


static PRUint64 certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy_LeafFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD 
  | CERT_REV_M_FORBID_NETWORK_FETCHING
  | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO,
  /* ocsp */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO
};

static PRUint64 certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy_ChainFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FORBID_NETWORK_FETCHING
  | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO,
  /* ocsp */
  0
};

static CERTRevocationMethodIndex 
certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy_Method_Preference = {
  cert_revocation_method_crl
};

static const CERTRevocationFlags certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy = {
  {
    /* leafTests */
    2,
    certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy_LeafFlags,
    1,
    &certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy_Method_Preference,
    0
  },
  {
    /* chainTests */
    2,
    certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy_ChainFlags,
    0,
    0,
    0
  }
};

extern const CERTRevocationFlags*
CERT_GetClassicOCSPEnabledHardFailurePolicy()
{
    return &certRev_NSS_3_11_Ocsp_Enabled_Hard_Policy;
}


static PRUint64 certRev_NSS_3_11_Ocsp_Disabled_Policy_LeafFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FORBID_NETWORK_FETCHING
  | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO,
  /* ocsp */
  0
};

static PRUint64 certRev_NSS_3_11_Ocsp_Disabled_Policy_ChainFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FORBID_NETWORK_FETCHING
  | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO,
  /* ocsp */
  0
};

static const CERTRevocationFlags certRev_NSS_3_11_Ocsp_Disabled_Policy = {
  {
    /* leafTests */
    2,
    certRev_NSS_3_11_Ocsp_Disabled_Policy_LeafFlags,
    0,
    0,
    0
  },
  {
    /* chainTests */
    2,
    certRev_NSS_3_11_Ocsp_Disabled_Policy_ChainFlags,
    0,
    0,
    0
  }
};

extern const CERTRevocationFlags*
CERT_GetClassicOCSPDisabledPolicy()
{
    return &certRev_NSS_3_11_Ocsp_Disabled_Policy;
}


static PRUint64 certRev_PKIX_Verify_Nist_Policy_LeafFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO
  | CERT_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE,
  /* ocsp */
  0
};

static PRUint64 certRev_PKIX_Verify_Nist_Policy_ChainFlags[2] = {
  /* crl */
  CERT_REV_M_TEST_USING_THIS_METHOD
  | CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO
  | CERT_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE,
  /* ocsp */
  0
};

static const CERTRevocationFlags certRev_PKIX_Verify_Nist_Policy = {
  {
    /* leafTests */
    2,
    certRev_PKIX_Verify_Nist_Policy_LeafFlags,
    0,
    0,
    0
  },
  {
    /* chainTests */
    2,
    certRev_PKIX_Verify_Nist_Policy_ChainFlags,
    0,
    0,
    0
  }
};

extern const CERTRevocationFlags*
CERT_GetPKIXVerifyNistRevocationPolicy()
{
    return &certRev_PKIX_Verify_Nist_Policy;
}


/*
 * CERT_PKIXVerifyCert
 *
 * Verify a Certificate using the PKIX library.
 *
 * Parameters:
 *  cert    - the target certificate to verify. Must be non-null
 *  params  - an array of type/value parameters which can be
 *            used to modify the behavior of the validation
 *            algorithm, or supply additional constraints.
 *
 *  outputTrustAnchor - the trust anchor which the certificate
 *                      chains to. The caller is responsible
 *                      for freeing this.
 *
 * Example Usage:
 *    CERTValParam args[3];
 *    args[0].type = cvpt_policyOID;
 *    args[0].value.si = oid;
 *    args[1].type = revCheckRequired;
 *    args[1].value.b = PR_TRUE;
 *    args[2].type = cvpt_end;
 *
 *    CERT_PKIXVerifyCert(cert, &output, args
 */
SECStatus CERT_PKIXVerifyCert(
 CERTCertificate *cert,
 SECCertificateUsage usages,
 CERTValInParam *paramsIn,
 CERTValOutParam *paramsOut,
 void *wincx)
{
    SECStatus             r = SECFailure;
    PKIX_Error *          error = NULL;
    PKIX_ProcessingParams *procParams = NULL;
    PKIX_BuildResult *    buildResult = NULL;
    void *                nbioContext = NULL;  /* for non-blocking IO */
    void *                buildState = NULL;   /* for non-blocking IO */
    PKIX_CertSelector *   certSelector = NULL;
    PKIX_List *           certStores = NULL;
    PKIX_ValidateResult * valResult = NULL;
    PKIX_VerifyNode     * verifyNode = NULL;
    PKIX_TrustAnchor *    trustAnchor = NULL;
    PKIX_PL_Cert *        trustAnchorCert = NULL;
    PKIX_List *           builtCertList = NULL;
    CERTValOutParam *     oparam = NULL;
    int i=0;

    void *plContext = NULL;

#ifdef PKIX_OBJECT_LEAK_TEST
    int  leakedObjNum = 0;
    int  memLeakLoopCount = 0;
    int  objCountTable[PKIX_NUMTYPES];
    int  fnInvLocalCount = 0;
    fnStackNameArr[0] = "CERT_PKIXVerifyCert";
    fnStackInvCountArr[0] = 0;
    PKIX_Boolean abortOnLeak = 
        PR_GetEnv("PKIX_OBJECT_LEAK_TEST_ABORT_ON_LEAK") ?
                                                   PKIX_FALSE : PKIX_TRUE;
    runningLeakTest = PKIX_TRUE;

    /* Prevent multi-threaded run of object leak test */
    fnInvLocalCount = PR_AtomicIncrement(&parallelFnInvocationCount);
    PORT_Assert(fnInvLocalCount == 1);

do {
    r = SECFailure;
    error = NULL;
    procParams = NULL;
    buildResult = NULL;
    nbioContext = NULL;  /* for non-blocking IO */
    buildState = NULL;   /* for non-blocking IO */
    certSelector = NULL;
    certStores = NULL;
    valResult = NULL;
    trustAnchor = NULL;
    trustAnchorCert = NULL;
    oparam = NULL;
    i=0;
    errorGenerated = PKIX_FALSE;
    stackPosition = 0;

    if (leakedObjNum) {
        pkix_pl_lifecycle_ObjectTableUpdate(objCountTable);
    }

    PR_LOG(pkixLog, 1, ("Memory leak test: Loop %d\n", memLeakLoopCount));
#endif /* PKIX_OBJECT_LEAK_TEST */

    error = PKIX_PL_NssContext_Create(
            0, PR_FALSE /*use arena*/, wincx, &plContext);
    if (error != NULL) {        /* need pkix->nss error map */
        PORT_SetError(SEC_ERROR_CERT_NOT_VALID);
        goto cleanup;
    }

    error = pkix_pl_NssContext_SetCertUsage(usages, plContext);
    if (error != NULL) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        goto cleanup;
    }

    error = PKIX_ProcessingParams_Create(&procParams, plContext);
    if (error != NULL) {              /* need pkix->nss error map */
        PORT_SetError(SEC_ERROR_CERT_NOT_VALID);
        goto cleanup;
    }


    /* now process the extensible input parameters structure */
    if (paramsIn != NULL) {
        i=0;
        while (paramsIn[i].type != cert_pi_end) {
            if (paramsIn[i].type >= cert_pi_max) {
                PORT_SetError(SEC_ERROR_INVALID_ARGS);
                goto cleanup;
            }
            if (cert_pkixSetParam(procParams,
                     &paramsIn[i],plContext) != SECSuccess) {
                PORT_SetError(SEC_ERROR_INVALID_ARGS);
                goto cleanup;
            }
            i++;
        }
    }


    certSelector = cert_GetTargetCertConstraints(cert, plContext);
    if (certSelector == NULL) {
        goto cleanup;
    }
    error = PKIX_ProcessingParams_SetTargetCertConstraints
        (procParams, certSelector, plContext);
    if (error != NULL) {
        goto cleanup;
    }

    certStores = cert_GetCertStores(plContext);
    if (certStores == NULL) {
        goto cleanup;
    }
    error = PKIX_ProcessingParams_SetCertStores
        (procParams, certStores, plContext);
    if (error != NULL) {
        goto cleanup;
    }

    error = PKIX_BuildChain( procParams, &nbioContext,
                             &buildState, &buildResult, &verifyNode,
                             plContext);
    if (error != NULL) {
        goto cleanup;
    }

    error = PKIX_BuildResult_GetValidateResult( buildResult, &valResult,
                                                plContext);
    if (error != NULL) {
        goto cleanup;
    }

    error = PKIX_ValidateResult_GetTrustAnchor( valResult, &trustAnchor,
                                                plContext);
    if (error != NULL) {
        goto cleanup;
    }

    error = PKIX_TrustAnchor_GetTrustedCert( trustAnchor, &trustAnchorCert,
                                                plContext);
    if (error != NULL) {
        goto cleanup;
    }

    oparam = cert_pkix_FindOutputParam(paramsOut, cert_po_trustAnchor);
    if (oparam != NULL) {
        oparam->value.pointer.cert = 
                cert_NSSCertFromPKIXCert(trustAnchorCert,plContext);
    }

    error = PKIX_BuildResult_GetCertChain( buildResult, &builtCertList,
                                                plContext);
    if (error != NULL) {
        goto cleanup;
    }

    oparam = cert_pkix_FindOutputParam(paramsOut, cert_po_certList);
    if (oparam != NULL) {
        error = cert_PkixToNssCertsChain(builtCertList,
                                         &oparam->value.pointer.chain,
                                         plContext);
        if (error) goto cleanup;
    }

    r = SECSuccess;

cleanup:
    if (verifyNode) {
        /* Return validation log only upon error. */
        oparam = cert_pkix_FindOutputParam(paramsOut, cert_po_errorLog);
        if (r && oparam != NULL) {
            PKIX_Error *tmpError =
                cert_GetLogFromVerifyNode(oparam->value.pointer.log,
                                          verifyNode, plContext);
            if (tmpError) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)tmpError, plContext);
            }
        }
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)verifyNode, plContext);
    }

    if (procParams != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)procParams, plContext);

    if (trustAnchorCert != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)trustAnchorCert, plContext);

    if (trustAnchor != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)trustAnchor, plContext);

    if (valResult != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)valResult, plContext);

    if (buildResult != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)buildResult, plContext);

    if (certStores != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)certStores, plContext);

    if (certSelector != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)certSelector, plContext);

    if (builtCertList != NULL) 
       PKIX_PL_Object_DecRef((PKIX_PL_Object *)builtCertList, plContext);

    if (error != NULL) {
        SECErrorCodes         nssErrorCode = 0;

        cert_PkixErrorToNssCode(error, &nssErrorCode, plContext);
        cert_pkixDestroyValOutParam(paramsOut);
        PORT_SetError(nssErrorCode);
        PKIX_PL_Object_DecRef((PKIX_PL_Object *)error, plContext);
    }

    PKIX_PL_NssContext_Destroy(plContext);

#ifdef PKIX_OBJECT_LEAK_TEST
    leakedObjNum =
        pkix_pl_lifecycle_ObjectLeakCheck(leakedObjNum ? objCountTable : NULL);

    if (abortOnLeak) {
        PORT_Assert(leakedObjNum == 0);
    }
    
} while (errorGenerated);

    runningLeakTest = PKIX_FALSE; 
    PR_AtomicDecrement(&parallelFnInvocationCount);
#endif /* PKIX_OBJECT_LEAK_TEST */

    return r;
}

