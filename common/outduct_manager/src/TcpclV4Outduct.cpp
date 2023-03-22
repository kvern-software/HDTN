/**
 * @file TcpclV4Outduct.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright � 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "TcpclV4Outduct.h"
#include "Logger.h"
#include <boost/make_unique.hpp>
#include <memory>
#include <boost/lexical_cast.hpp>
#include "Uri.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

TcpclV4Outduct::TcpclV4Outduct(const outduct_element_config_t & outductConfig, const uint64_t myNodeId, const uint64_t outductUuid,
    const uint64_t maxOpportunisticRxBundleSizeBytes,
    const OutductOpportunisticProcessReceivedBundleCallback_t & outductOpportunisticProcessReceivedBundleCallback) :
    Outduct(outductConfig, outductUuid),

#ifdef OPENSSL_SUPPORT_ENABLED

 //tls version 1.3 requires boost 1.69 beta 1 or greater
#if (BOOST_VERSION >= 106900)
    m_shareableSslContext((outductConfig.useTlsVersion1_3) ? boost::asio::ssl::context::tlsv13_client : boost::asio::ssl::context::tlsv12_client),
#else
    m_shareableSslContext(boost::asio::ssl::context::tlsv12_client),
#endif

    m_tcpclV4BundleSource(m_shareableSslContext,
#else
    m_tcpclV4BundleSource(
#endif
        outductConfig.tryUseTls, outductConfig.tlsIsRequired,
        outductConfig.keepAliveIntervalSeconds, myNodeId,
        Uri::GetIpnUriString(outductConfig.nextHopNodeId, 0), //ion 3.7.2 source code tcpcli.c line 1199 uses service number 0 for contact header:
        outductConfig.maxNumberOfBundlesInPipeline + 5, outductConfig.tcpclV4MyMaxRxSegmentSizeBytes, maxOpportunisticRxBundleSizeBytes, outductOpportunisticProcessReceivedBundleCallback)
{
#ifdef OPENSSL_SUPPORT_ENABLED
    if (outductConfig.tryUseTls) {
#if (BOOST_VERSION < 106900)
        if (outductConfig.useTlsVersion1_3) {
            LOG_WARNING(subprocess) << "TLS Version 1.3 was specified but that requires compiling with boost version 1.69.0-beta1 or greater.. using TLS Version 1.2 instead.";
        }
#endif
        try {
            m_shareableSslContext.load_verify_file(outductConfig.certificationAuthorityPemFileForVerification.string());//"C:/hdtn_ssl_certificates/cert.pem");
            m_shareableSslContext.set_verify_mode(boost::asio::ssl::verify_peer);
        }
        catch (boost::system::system_error & e) {
            LOG_ERROR(subprocess) << "TcpclV4Outduct constructor: " << e.what();
            return;
        }
        //ion 3.7.2 source code tcpcli.c line 1199 uses service number 0 for contact header:
        //isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", getOwnNodeNbr());
        const std::string nextHopEndpointIdWithServiceIdZero = Uri::GetIpnUriString(outductConfig.nextHopNodeId, 0);
       
        m_shareableSslContext.set_verify_callback(
            boost::bind(&TcpclV4Outduct::VerifyCertificate, this, boost::placeholders::_1, boost::placeholders::_2, nextHopEndpointIdWithServiceIdZero,
                outductConfig.verifySubjectAltNameInX509Certificate, outductConfig.doX509CertificateVerification));
    }
#endif
}
TcpclV4Outduct::~TcpclV4Outduct() {}

std::size_t TcpclV4Outduct::GetTotalDataSegmentsUnacked() {
    return m_tcpclV4BundleSource.Virtual_GetTotalBundlesUnacked();
}
bool TcpclV4Outduct::Forward(const uint8_t* bundleData, const std::size_t size, std::vector<uint8_t>&& userData) {
    return m_tcpclV4BundleSource.BaseClass_Forward(bundleData, size, std::move(userData));
}
bool TcpclV4Outduct::Forward(zmq::message_t & movableDataZmq, std::vector<uint8_t>&& userData) {
    return m_tcpclV4BundleSource.BaseClass_Forward(movableDataZmq, std::move(userData));
}
bool TcpclV4Outduct::Forward(std::vector<uint8_t> & movableDataVec, std::vector<uint8_t>&& userData) {
    return m_tcpclV4BundleSource.BaseClass_Forward(movableDataVec, std::move(userData));
}

void TcpclV4Outduct::SetOnFailedBundleVecSendCallback(const OnFailedBundleVecSendCallback_t& callback) {
    m_tcpclV4BundleSource.BaseClass_SetOnFailedBundleVecSendCallback(callback);
}
void TcpclV4Outduct::SetOnFailedBundleZmqSendCallback(const OnFailedBundleZmqSendCallback_t& callback) {
    m_tcpclV4BundleSource.BaseClass_SetOnFailedBundleZmqSendCallback(callback);
}
void TcpclV4Outduct::SetOnSuccessfulBundleSendCallback(const OnSuccessfulBundleSendCallback_t& callback) {
    m_tcpclV4BundleSource.BaseClass_SetOnSuccessfulBundleSendCallback(callback);
}
void TcpclV4Outduct::SetOnOutductLinkStatusChangedCallback(const OnOutductLinkStatusChangedCallback_t& callback) {
    m_tcpclV4BundleSource.BaseClass_SetOnOutductLinkStatusChangedCallback(callback);
}
void TcpclV4Outduct::SetUserAssignedUuid(uint64_t userAssignedUuid) {
    m_tcpclV4BundleSource.BaseClass_SetUserAssignedUuid(userAssignedUuid);
}

void TcpclV4Outduct::Connect() {
    m_tcpclV4BundleSource.Connect(m_outductConfig.remoteHostname, boost::lexical_cast<std::string>(m_outductConfig.remotePort));
}
bool TcpclV4Outduct::ReadyToForward() {
    return m_tcpclV4BundleSource.ReadyToForward();
}
void TcpclV4Outduct::Stop() {
    m_tcpclV4BundleSource.Stop();
}
void TcpclV4Outduct::GetOutductFinalStats(OutductFinalStats & finalStats) {
    finalStats.m_convergenceLayer = m_outductConfig.convergenceLayer;
    finalStats.m_totalDataSegmentsOrPacketsAcked = m_tcpclV4BundleSource.Virtual_GetTotalBundlesAcked();
    finalStats.m_totalDataSegmentsOrPacketsSent = m_tcpclV4BundleSource.Virtual_GetTotalBundlesSent();
}
void TcpclV4Outduct::PopulateOutductTelemetry(std::unique_ptr<OutductTelemetry_t>& outductTelem) {
    outductTelem = boost::make_unique<TcpclV4OutductTelemetry_t>(m_tcpclV4BundleSource.m_base_outductTelemetry);
    outductTelem->m_linkIsUpPerTimeSchedule = m_linkIsUpPerTimeSchedule;
}


#ifdef OPENSSL_SUPPORT_ENABLED
static bool VerifySubjectAltNameFromCertificate(X509 *cert, const std::string & expectedIpnEidUri) {

    GENERAL_NAMES* altNames = (GENERAL_NAMES*)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    const int numAltNames = sk_GENERAL_NAME_num(altNames);

    for (int i = 0; i < numAltNames; ++i) {
        GENERAL_NAME *currentSubjectAltName = sk_GENERAL_NAME_value(altNames, i);

        if (currentSubjectAltName->type == GEN_URI) {
            unsigned char *outBuf = NULL;
            ASN1_STRING_to_UTF8(&outBuf, currentSubjectAltName->d.uniformResourceIdentifier);
            const std::string subjectAltNameString((const char *)outBuf, (std::size_t)ASN1_STRING_length(currentSubjectAltName->d.uniformResourceIdentifier));
            LOG_INFO(subprocess) << "subjectAltNameString=" << subjectAltNameString;
            OPENSSL_free(outBuf);
            if (subjectAltNameString == expectedIpnEidUri) {
                LOG_WARNING(subprocess) << "using an older draft version of tcpcl version 4 that uses URIs as the subject alternative name";
                LOG_WARNING(subprocess) << " switch to id-on-bundleEID when generating certificates";
                return true;
            }
        }
        else if (currentSubjectAltName->type == GEN_OTHERNAME) {
            unsigned char *outBuf = NULL;
            //http://oid-info.com/get/1.3.6.1.5.5.7.8.11
            static const int bundleEidNid = OBJ_create("1.3.6.1.5.5.7.8.11", "id-on-bundleEID", "BundleEID (See IETF RFC 9174)");
            int nid = OBJ_obj2nid(currentSubjectAltName->d.otherName->type_id);
            if (nid == bundleEidNid) {
                if (currentSubjectAltName->d.otherName->value->type == V_ASN1_IA5STRING) {
                    //ASN1_STRING_get0_data returns an internal pointer to the data of x. Since this is an internal pointer it should not be freed or modified in any way.
                    const char * ia5StringData = (const char*)ASN1_STRING_get0_data(currentSubjectAltName->d.otherName->value->value.ia5string);
                    const std::string uriString(ia5StringData);
                    LOG_INFO(subprocess) << "id-on-bundleEID=" << uriString;
                    if (uriString == expectedIpnEidUri) {
                        return true;
                    }
                }
                else {
                    LOG_WARNING(subprocess) << "unknown type in currentSubjectAltName->d.otherName->value->type, got " << currentSubjectAltName->d.otherName->value->type;
                }
            }
            else {
                LOG_WARNING(subprocess) << "unknown nid, got " << nid;
            }
        }
    }
    return false;
}

bool TcpclV4Outduct::VerifyCertificate(bool preverified, boost::asio::ssl::verify_context& ctx, const std::string & nextHopEndpointIdStrWithServiceIdZero, bool doVerifyNextHopEndpointIdStr, bool doX509CertificateVerification) {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // get the certificate's subject name.
    std::vector<char> subjectName(256);
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), &subjectName[0], (int)subjectName.size());

    //The TCPCL requires Version 3 certificates due to the extensions used
    //by this profile.  TCPCL entities SHALL reject as invalid Version 1
    //and Version 2 end-entity certificates.
   
    //X509_get_version() returns the numerical value of the version field of certificate x.
    //Note: this is defined by standards (X.509 et al) to be one less than the certificate version.
    //So a version 3 certificate will return 2 and a version 1 certificate will return 0.
    const long x509Version = X509_get_version(cert) + 1;
    if (doX509CertificateVerification) {
        LOG_INFO(subprocess) << "Verifying " << subjectName.data() << "  preverified=" << preverified << " x509 version=" << x509Version;
    }
    else {
        LOG_INFO(subprocess) << "Skipping verification and accepting this certificate: subject=" << subjectName.data() << "  preverified=" << preverified << " x509 version=" << x509Version;
        return true;
    }
    if (x509Version < 3) {
        LOG_ERROR(subprocess) << "TcpclV4Outduct::VerifyCertificate: tcpclV4 requires a minimum X.509 certificate of 3 but got " << x509Version;
        return false;
    }
    if (!preverified) {
        LOG_ERROR(subprocess) << "TcpclV4Outduct::VerifyCertificate: X.509 certificate not verified";
        return false;
    }
    
    
    if (doVerifyNextHopEndpointIdStr) {
        if (!VerifySubjectAltNameFromCertificate(cert, nextHopEndpointIdStrWithServiceIdZero)) {
            LOG_ERROR(subprocess) << "TcpclV4Outduct::VerifyCertificate: the subjectAltName URI in the X.509 certificate does not match the next hop endpoint id of " << nextHopEndpointIdStrWithServiceIdZero;
            return false;
        }
        else {
            LOG_INFO(subprocess) << "success: X.509 certificate subjectAltName matches the nextHopEndpointIdStr";
        }
    }

    return true;
}


#endif
