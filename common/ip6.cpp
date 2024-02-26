/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "ip6.h"

#include "uint128.h"
#include <QHostAddress>
#include <QRandomGenerator>


Ip6Protocol::Ip6Protocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

Ip6Protocol::~Ip6Protocol()
{
}

AbstractProtocol* Ip6Protocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new Ip6Protocol(stream, parent);
}

quint32 Ip6Protocol::protocolNumber() const
{
    return OstProto::Protocol::kIp6FieldNumber;
}

void Ip6Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::ip6)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Ip6Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::ip6))
        data.MergeFrom(protocol.GetExtension(OstProto::ip6));
}

QString Ip6Protocol::name() const
{
    return QString("Internet Protocol ver 6");
}

QString Ip6Protocol::shortName() const
{
    return QString("IPv6");
}

AbstractProtocol::ProtocolIdType Ip6Protocol::protocolIdType() const
{
    return ProtocolIdIp;
}

quint32 Ip6Protocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdEth: return 0x86dd;
        case ProtocolIdIp: return 0x29;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int Ip6Protocol::fieldCount() const
{
    return ip6_fieldCount;
}

AbstractProtocol::FieldFlags Ip6Protocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case ip6_version:
        case ip6_trafficClass:
        case ip6_flowLabel:
        case ip6_payloadLength:
        case ip6_nextHeader:
        case ip6_hopLimit:
        case ip6_srcAddress:
        case ip6_dstAddress:
            break;

        case ip6_isOverrideVersion:
        case ip6_isOverridePayloadLength:
        case ip6_isOverrideNextHeader:

        case ip6_srcAddrMode:
        case ip6_srcAddrCount:
        case ip6_srcAddrPrefix:

        case ip6_dstAddrMode:
        case ip6_dstAddrCount:
        case ip6_dstAddrPrefix:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant Ip6Protocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case ip6_version:
        {
            quint8 ver;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_version())
                        ver = data.version() & 0xF;
                    else
                        ver = 0x6;
                    break;
                default:
                    ver = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Version");
                case FieldValue:
                    return ver;
                case FieldTextValue:
                    return QString("%1").arg(ver);
                case FieldFrameValue:
                    return QByteArray(1, char(ver));
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;
        }
        case ip6_trafficClass:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Traffic Class");
                case FieldValue:
                    return data.traffic_class() & 0xFF;
                case FieldTextValue:
                    return QString("%1").arg(data.traffic_class() & 0xFF, 
                            2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, char(data.traffic_class() & 0xFF));
                default:
                    break;
            }
            break;
        }
        case ip6_flowLabel:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Flow Label");
                case FieldValue:
                    return data.flow_label() & 0xFFFFF;
                case FieldTextValue:
                    return QString("%1").arg(data.flow_label() & 0xFFFFF, 
                            5, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.flow_label() & 0xFFFFF, 
                            (uchar*) fv.data());
                    fv = fv.right(3);
                    return fv;
                }
                case FieldBitSize:
                    return 20;
                default:
                    break;
            }
            break;
        }
        case ip6_payloadLength:
        {
            quint16 len;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_payload_length())
                        len = data.payload_length();
                    else
                        len = protocolFramePayloadSize(streamIndex);
                    break;
                default:
                    len = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Payload Length");
                case FieldValue:
                    return len;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(len, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QString("%1").arg(len);
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case ip6_nextHeader:
        {
            quint8 nextHdr;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_next_header()) {
                        nextHdr = data.next_header();
                    }
                    else {
                        nextHdr = payloadProtocolId(ProtocolIdIp);
                        if ((nextHdr == 0) 
                                && next 
                                && (next->protocolIdType() == ProtocolIdNone)) {
                            nextHdr = 0x3b; // IPv6 No-Next-Header
                        }
                    }
                    break;
                default:
                    nextHdr = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Next Header");
                case FieldValue:
                    return nextHdr;
                case FieldTextValue:
                    return QString("%1").arg(nextHdr, 2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, char(nextHdr));
                default:
                    break;
            }
            break;
        }
        case ip6_hopLimit:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Hop Limit");
                case FieldValue:
                    return data.hop_limit() & 0xFF;
                case FieldTextValue:
                    return QString("%1").arg(data.hop_limit() & 0xFF);
                case FieldFrameValue:
                    return QByteArray(1, char(data.hop_limit() & 0xFF));
                default:
                    break;
            }
            break;
        }

        case ip6_srcAddress:
        {
            int u;
            UInt128 mask = 0;
            UInt128 prefix = 0;
            UInt128 host = 0;
            UInt128 src(data.src_addr_hi(), data.src_addr_lo());

            switch(data.src_addr_mode())
            {
                case OstProto::Ip6::kFixed:
                    break;
                case OstProto::Ip6::kIncHost:
                case OstProto::Ip6::kDecHost:
                case OstProto::Ip6::kRandomHost:
                    u = streamIndex % data.src_addr_count();
                    mask = ~UInt128(0, 0) << (128 - data.src_addr_prefix());
                    prefix = src & mask;
                    if (data.src_addr_mode() == OstProto::Ip6::kIncHost) {
                        host = ((src & ~mask) + u) & ~mask;
                    } 
                    else if (data.src_addr_mode() == OstProto::Ip6::kDecHost) {
                        host = ((src & ~mask) - u) & ~mask;
                    } 
                    else if (data.src_addr_mode()==OstProto::Ip6::kRandomHost) {
                        // XXX: qrand is int (32bit) not 64bit, some stdlib
                        // implementations have RAND_MAX as low as 0x7FFF
                        host = UInt128(QRandomGenerator::global()->generate(), QRandomGenerator::global()->generate()) & ~mask;
                    }
                    src = prefix | host;
                    break;
                default:
                    qWarning("Unhandled src_addr_mode = %d", 
                            data.src_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Source");
                case FieldValue:
                {
                    QVariant v;
                    v.setValue(src);
                    return v;
                }
                case FieldFrameValue:
                case FieldTextValue:
                {
                    QByteArray fv;
                    fv.resize(16);
                    qToBigEndian(src, (uchar*) fv.data());
                    if (attrib == FieldTextValue)
                        return QHostAddress(src.toArray()).toString();
                    else
                        return fv;
                }
                default:
                    break;
            }
            break;
        }

        case ip6_dstAddress:
        {
            int u;
            UInt128 mask = 0;
            UInt128 prefix = 0;
            UInt128 host = 0;
            UInt128 dst(data.dst_addr_hi(), data.dst_addr_lo());

            switch(data.dst_addr_mode())
            {
                case OstProto::Ip6::kFixed:
                    break;
                case OstProto::Ip6::kIncHost:
                case OstProto::Ip6::kDecHost:
                case OstProto::Ip6::kRandomHost:
                    u = streamIndex % data.dst_addr_count();
                    mask = ~UInt128(0, 0) << (128 - data.dst_addr_prefix());
                    prefix = dst & mask;
                    if (data.dst_addr_mode() == OstProto::Ip6::kIncHost) {
                        host = ((dst & ~mask) + u) & ~mask;
                    } 
                    else if (data.dst_addr_mode() == OstProto::Ip6::kDecHost) {
                        host = ((dst & ~mask) - u) & ~mask;
                    } 
                    else if (data.dst_addr_mode()==OstProto::Ip6::kRandomHost) {
                        // XXX: qrand is int (32bit) not 64bit, some stdlib
                        // implementations have RAND_MAX as low as 0x7FFF
                        host = UInt128(QRandomGenerator::global()->generate(), QRandomGenerator::global()->generate()) & ~mask;
                    }
                    dst = prefix | host;
                    break;
                default:
                    qWarning("Unhandled dst_addr_mode = %d", 
                            data.dst_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Destination");
                case FieldValue:
                {
                    QVariant v;
                    v.setValue(dst);
                    return v;
                }
                case FieldFrameValue:
                case FieldTextValue:
                {
                    QByteArray fv;
                    fv.resize(16);
                    qToBigEndian(dst, (uchar*) fv.data());
                    if (attrib == FieldTextValue)
                        return QHostAddress(dst.toArray()).toString();
                    else
                        return fv;
                }
                default:
                    break;
            }
            break;
        }

        // Meta-Fields
        case ip6_isOverrideVersion:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_version();
                default:
                    break;
            }
            break;
        }
        case ip6_isOverridePayloadLength:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_payload_length();
                default:
                    break;
            }
            break;
        }
        case ip6_isOverrideNextHeader:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_next_header();
                default:
                    break;
            }
            break;
        }

        case ip6_srcAddrMode:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.src_addr_mode();
                default:
                    break;
            }
            break;
        }
        case ip6_srcAddrCount:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.src_addr_count();
                default:
                    break;
            }
            break;
        }
        case ip6_srcAddrPrefix:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.src_addr_prefix();
                default:
                    break;
            }
            break;
        }

        case ip6_dstAddrMode:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.dst_addr_mode();
                default:
                    break;
            }
            break;
        }
        case ip6_dstAddrCount:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.dst_addr_count();
                default:
                    break;
            }
            break;
        }
        case ip6_dstAddrPrefix:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.dst_addr_prefix();
                default:
                    break;
            }
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool Ip6Protocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case ip6_version:
        {
            uint ver = value.toUInt(&isOk);
            if (isOk)
                data.set_version(ver & 0xF);
            break;
        }
        case ip6_trafficClass:
        {
            uint trfClass = value.toUInt(&isOk);
            if (isOk)
                data.set_traffic_class(trfClass & 0xFF);
            break;
        }
        case ip6_flowLabel:
        {
            uint fl = value.toUInt(&isOk);
            if (isOk)
                data.set_flow_label(fl & 0xFFFFF);
            break;
        }
        case ip6_payloadLength:
        {
            uint len = value.toUInt(&isOk);
            if (isOk)
                data.set_payload_length(len & 0xFFFF);
            break;
        }
        case ip6_nextHeader:
        {
            uint ver = value.toUInt(&isOk);
            if (isOk)
                data.set_next_header(ver & 0xFF);
            break;
        }
        case ip6_hopLimit:
        {
            uint hl = value.toUInt(&isOk);
            if (isOk)
                data.set_hop_limit(hl & 0xFF);
            break;
        }
        case ip6_srcAddress:
        {
            if (value.typeName() == QString("UInt128")) {
                UInt128 addr = value.value<UInt128>();
                data.set_src_addr_hi(addr.hi64());
                data.set_src_addr_lo(addr.lo64());
                isOk = true;
                break;
            }

            Q_IPV6ADDR addr = QHostAddress(value.toString()).toIPv6Address();
            quint64 x;

            x =   (quint64(addr[0]) << 56)
                | (quint64(addr[1]) << 48)
                | (quint64(addr[2]) << 40)
                | (quint64(addr[3]) << 32)
                | (quint64(addr[4]) << 24)
                | (quint64(addr[5]) << 16)
                | (quint64(addr[6]) <<  8)
                | (quint64(addr[7]) <<  0);
            data.set_src_addr_hi(x);

            x =   (quint64(addr[ 8]) << 56)
                | (quint64(addr[ 9]) << 48)
                | (quint64(addr[10]) << 40)
                | (quint64(addr[11]) << 32)
                | (quint64(addr[12]) << 24)
                | (quint64(addr[13]) << 16)
                | (quint64(addr[14]) <<  8)
                | (quint64(addr[15]) <<  0);
            data.set_src_addr_lo(x);
            isOk = true;
            break;
        }
        case ip6_dstAddress:
        {
            if (value.typeName() == QString("UInt128")) {
                UInt128 addr = value.value<UInt128>();
                data.set_dst_addr_hi(addr.hi64());
                data.set_dst_addr_lo(addr.lo64());
                isOk = true;
                break;
            }

            Q_IPV6ADDR addr = QHostAddress(value.toString()).toIPv6Address();
            quint64 x;

            x =   (quint64(addr[0]) << 56)
                | (quint64(addr[1]) << 48)
                | (quint64(addr[2]) << 40)
                | (quint64(addr[3]) << 32)
                | (quint64(addr[4]) << 24)
                | (quint64(addr[5]) << 16)
                | (quint64(addr[6]) <<  8)
                | (quint64(addr[7]) <<  0);
            data.set_dst_addr_hi(x);

            x =   (quint64(addr[ 8]) << 56)
                | (quint64(addr[ 9]) << 48)
                | (quint64(addr[10]) << 40)
                | (quint64(addr[11]) << 32)
                | (quint64(addr[12]) << 24)
                | (quint64(addr[13]) << 16)
                | (quint64(addr[14]) <<  8)
                | (quint64(addr[15]) <<  0);
            data.set_dst_addr_lo(x);
            isOk = true;
            break;
        }

        // Meta-Fields
        case ip6_isOverrideVersion:
        {
            bool ovr = value.toBool();
            data.set_is_override_version(ovr);
            isOk = true;
            break;
        }
        case ip6_isOverridePayloadLength:
        {
            bool ovr = value.toBool();
            data.set_is_override_payload_length(ovr);
            isOk = true;
            break;
        }
        case ip6_isOverrideNextHeader:
        {
            bool ovr = value.toBool();
            data.set_is_override_next_header(ovr);
            isOk = true;
            break;
        }

        case ip6_srcAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.AddrMode_IsValid(mode))
                data.set_src_addr_mode((OstProto::Ip6::AddrMode) mode);
            else
                isOk = false;
            break;
        }
        case ip6_srcAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_src_addr_count(count);
            break;
        }
        case ip6_srcAddrPrefix:
        {
            uint prefix = value.toUInt(&isOk);
            if (isOk)
                data.set_src_addr_prefix(prefix);
            break;
        }

        case ip6_dstAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.AddrMode_IsValid(mode))
                data.set_dst_addr_mode((OstProto::Ip6::AddrMode) mode);
            else
                isOk = false;
            break;
        }
        case ip6_dstAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_addr_count(count);
            break;
        }
        case ip6_dstAddrPrefix:
        {
            uint prefix = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_addr_prefix(prefix);
            break;
        }

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

_exit:
    return isOk;
}

int Ip6Protocol::protocolFrameVariableCount() const
{
    int count = AbstractProtocol::protocolFrameVariableCount();

    if (data.src_addr_mode() != OstProto::Ip6::kFixed)
        count = AbstractProtocol::lcm(count, data.src_addr_count());

    if (data.dst_addr_mode() != OstProto::Ip6::kFixed)
        count = AbstractProtocol::lcm(count, data.dst_addr_count());

    return count;
}

quint32 Ip6Protocol::protocolFrameCksum(int streamIndex, 
        CksumType cksumType, CksumFlags cksumFlags) const
{
    if (cksumType == CksumIpPseudo)
    {
        quint32 sum = 0;
        QByteArray fv = protocolFrameValue(streamIndex);
        const quint8 *p = (quint8*) fv.constData();

        // src-ip, dst-ip
        for (int i = 8; i < fv.size(); i+=2)
            sum += *((quint16*)(p + i));

        // XXX: payload length and protocol are also part of the
        // pseudo cksum but we need to skip extension headers to
        // get to them as per RFC 8200 Section 8.1
        // Since we can't traverse beyond our immediate neighboring
        // protocol from here, these two fields are counted in the
        // pseudo cksum in AbstractProtocol::protocolFrameHeaderCksum()

        while(sum>>16)
            sum = (sum & 0xFFFF) + (sum >> 16);

        return qFromBigEndian((quint16) ~sum);
    }
    return AbstractProtocol::protocolFrameCksum(
                                streamIndex, cksumType, cksumFlags);
}

bool Ip6Protocol::hasErrors(QStringList *errors) const
{
    bool result = false;

    if ((data.dst_addr_hi() == 0ULL) && (data.dst_addr_lo() == 0ULL)
            && (data.dst_addr_mode() == OstProto::Ip6::kFixed)) {
        if (errors)
            *errors << QObject::tr("Frames with Destination IP :: (all zeroes) "
                                   "are likely to be dropped");
        result = true;
    }

    if ((data.src_addr_hi() == 0ULL) && (data.src_addr_lo() == 0ULL)
            && (data.src_addr_mode() == OstProto::Ip6::kFixed)) {
        if (errors)
            *errors << QObject::tr("Frames with Source IP :: (all zeroes) "
                                   "are likely to be dropped");
        result = true;
    }

    return result;
}
