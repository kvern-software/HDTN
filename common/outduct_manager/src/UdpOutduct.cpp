/**
 * @file UdpOutduct.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright © 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "UdpOutduct.h"
#include <iostream>
#include <boost/make_unique.hpp>
#include <memory>
#include <boost/lexical_cast.hpp>

UdpOutduct::UdpOutduct(const outduct_element_config_t & outductConfig, const uint64_t outductUuid) :
    Outduct(outductConfig, outductUuid),
    m_udpBundleSource(outductConfig.udpRateBps, outductConfig.maxNumberOfBundlesInPipeline + 5)
{}
UdpOutduct::~UdpOutduct() {}

std::size_t UdpOutduct::GetTotalDataSegmentsUnacked() {
    return m_udpBundleSource.GetTotalUdpPacketsUnacked();
}
bool UdpOutduct::Forward(const uint8_t* bundleData, const std::size_t size, std::vector<uint8_t>&& userData) {
    return m_udpBundleSource.Forward(bundleData, size, std::move(userData));
}
bool UdpOutduct::Forward(zmq::message_t & movableDataZmq, std::vector<uint8_t>&& userData) {
    return m_udpBundleSource.Forward(movableDataZmq, std::move(userData));
}
bool UdpOutduct::Forward(std::vector<uint8_t> & movableDataVec, std::vector<uint8_t>&& userData) {
    return m_udpBundleSource.Forward(movableDataVec, std::move(userData));
}

void UdpOutduct::SetOnFailedBundleVecSendCallback(const OnFailedBundleVecSendCallback_t& callback) {
    m_udpBundleSource.SetOnFailedBundleVecSendCallback(callback);
}
void UdpOutduct::SetOnFailedBundleZmqSendCallback(const OnFailedBundleZmqSendCallback_t& callback) {
    m_udpBundleSource.SetOnFailedBundleZmqSendCallback(callback);
}
void UdpOutduct::SetOnSuccessfulBundleSendCallback(const OnSuccessfulBundleSendCallback_t& callback) {
    m_udpBundleSource.SetOnSuccessfulBundleSendCallback(callback);
}
void UdpOutduct::SetOnOutductLinkStatusChangedCallback(const OnOutductLinkStatusChangedCallback_t& callback) {
    m_udpBundleSource.SetOnOutductLinkStatusChangedCallback(callback);
}
void UdpOutduct::SetUserAssignedUuid(uint64_t userAssignedUuid) {
    m_udpBundleSource.SetUserAssignedUuid(userAssignedUuid);
}
void UdpOutduct::SetRate(uint64_t maxSendRateBitsPerSecOrZeroToDisable) {
    m_udpBundleSource.UpdateRate(maxSendRateBitsPerSecOrZeroToDisable);
}

void UdpOutduct::Connect() {
    m_udpBundleSource.Connect(m_outductConfig.remoteHostname, boost::lexical_cast<std::string>(m_outductConfig.remotePort));
}
bool UdpOutduct::ReadyToForward() {
    return m_udpBundleSource.ReadyToForward();
}
void UdpOutduct::Stop() {
    m_udpBundleSource.Stop();
}
void UdpOutduct::GetOutductFinalStats(OutductFinalStats & finalStats) {
    finalStats.m_convergenceLayer = m_outductConfig.convergenceLayer;
    finalStats.m_totalDataSegmentsOrPacketsAcked = m_udpBundleSource.GetTotalUdpPacketsAcked();
    finalStats.m_totalDataSegmentsOrPacketsSent = m_udpBundleSource.GetTotalUdpPacketsSent();
}
uint64_t UdpOutduct::GetStartingMaxSendRateBitsPerSec() const noexcept {
    return m_outductConfig.udpRateBps;
}
void UdpOutduct::PopulateOutductTelemetry(std::unique_ptr<OutductTelemetry_t>& outductTelem) {
    outductTelem = boost::make_unique<UdpOutductTelemetry_t>(m_udpBundleSource.m_udpOutductTelemetry);
    outductTelem->m_linkIsUpPerTimeSchedule = m_linkIsUpPerTimeSchedule;
}
