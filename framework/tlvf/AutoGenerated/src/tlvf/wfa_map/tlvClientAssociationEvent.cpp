///////////////////////////////////////
// AUTO GENERATED FILE - DO NOT EDIT //
///////////////////////////////////////

/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Copyright (c) 2016-2019 Intel Corporation
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <tlvf/wfa_map/tlvClientAssociationEvent.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvClientAssociationEvent::tlvClientAssociationEvent(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvClientAssociationEvent::tlvClientAssociationEvent(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvClientAssociationEvent::~tlvClientAssociationEvent() {
}
const eTlvTypeMap& tlvClientAssociationEvent::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvClientAssociationEvent::length() {
    return (const uint16_t&)(*m_length);
}

sMacAddr& tlvClientAssociationEvent::client_mac() {
    return (sMacAddr&)(*m_client_mac);
}

sMacAddr& tlvClientAssociationEvent::bssid() {
    return (sMacAddr&)(*m_bssid);
}

tlvClientAssociationEvent::eAssociationEvent& tlvClientAssociationEvent::association_event() {
    return (eAssociationEvent&)(*m_association_event);
}

void tlvClientAssociationEvent::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    m_client_mac->struct_swap();
    m_bssid->struct_swap();
    tlvf_swap(8*sizeof(eAssociationEvent), reinterpret_cast<uint8_t*>(m_association_event));
}

bool tlvClientAssociationEvent::finalize()
{
    if (m_parse__) {
        TLVF_LOG(DEBUG) << "finalize() called but m_parse__ is set";
        return true;
    }
    if (m_finalized__) {
        TLVF_LOG(DEBUG) << "finalize() called for already finalized class";
        return true;
    }
    if (!isPostInitSucceeded()) {
        TLVF_LOG(ERROR) << "post init check failed";
        return false;
    }
    if (m_inner__) {
        if (!m_inner__->finalize()) {
            TLVF_LOG(ERROR) << "m_inner__->finalize() failed";
            return false;
        }
        auto tailroom = m_inner__->getMessageBuffLength() - m_inner__->getMessageLength();
        m_buff_ptr__ -= tailroom;
        *m_length -= tailroom;
    }
    class_swap();
    m_finalized__ = true;
    return true;
}

size_t tlvClientAssociationEvent::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(sMacAddr); // client_mac
    class_size += sizeof(sMacAddr); // bssid
    class_size += sizeof(eAssociationEvent); // association_event
    return class_size;
}

bool tlvClientAssociationEvent::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = (eTlvTypeMap*)m_buff_ptr__;
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_CLIENT_ASSOCIATION_EVENT;
    if (!buffPtrIncrementSafe(sizeof(eTlvTypeMap))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eTlvTypeMap) << ") Failed!";
        return false;
    }
    m_length = (uint16_t*)m_buff_ptr__;
    if (!m_parse__) *m_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    m_client_mac = (sMacAddr*)m_buff_ptr__;
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sMacAddr); }
    if (!m_parse__) { m_client_mac->struct_init(); }
    m_bssid = (sMacAddr*)m_buff_ptr__;
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sMacAddr); }
    if (!m_parse__) { m_bssid->struct_init(); }
    m_association_event = (eAssociationEvent*)m_buff_ptr__;
    if (!buffPtrIncrementSafe(sizeof(eAssociationEvent))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eAssociationEvent) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(eAssociationEvent); }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_CLIENT_ASSOCIATION_EVENT) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_CLIENT_ASSOCIATION_EVENT) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


