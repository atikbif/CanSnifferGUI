#include "canrequest.h"
#include <QDebug>
#include "archiveanalyzer.h"
#include <QDateTime>

const std::array<QString,8> CanRequest::service = {
    "Channel",
    "Heartbeat",
    "Download",
    "Spare",
    "Write",
    "Read",
    "Action",
    "Event"
};

const std::array<QString,8> CanRequest::ssDefinition = {
    "REQUEST (not fragmented)",
    "REQUEST (fragmented)",
    "RESPONSE (successful, not fragmented)",
    "RESPONSE (successful, fragmented)",
    "RESPONSE (error, not fragmented)",
    "RESPONSE (error, fragmented)",
    "RESPONSE (wait, not fragmented)",
    "RESPONSE (wait, fragmented)"
};

QString CanRequest::getIOType(int num)
{
    QString res;
    switch(num) {
        case 0:res = " Analogue I/P";break;
        case 1:res = " Digital I/P";break;
        case 2:res = " Switch I/P";break;
        case 3:res = " Relay O/P";break;
        case 4:res = "Analogue O/P";break;
    }
    return res;
}

CanRequest::CanRequest(quint16 id, quint32 reqTime, const QByteArray data):id(id),reqTime(reqTime),data(data)
{

}

int CanRequest::getSS() const
{
    if(data.count()) {
        return (quint8)data.at(0)>>5;
    }
    return 0;
}

int CanRequest::getEOID() const
{
    if(data.count()) {
        return (quint8)data.at(0)&0x1F;
    }
    return 0;
}

int CanRequest::getAddress() const
{
    if(data.count()>=2) {
        return (quint8)data.at(1);
    }
    return 0;
}

QByteArray CanRequest::getPC21Data() const
{
    if(data.count()>2) {
        return data.mid(2);
    }
    return QByteArray();
}

QString CanRequest::getService() const
{
    QString res = "";
    int serv_num = (quint8)id & 0x07;
    try {
        res = service.at(serv_num);
    }catch(std::out_of_range) {
        qDebug() << "out of range exception num:" << serv_num;
    }
    return res;
}

QString CanRequest::getComment() const
{
    QString result;
    QTextStream stream(&result);
    int ss = getSS();
    try {
        stream.setFieldWidth(40);stream.setFieldAlignment(QTextStream::AlignLeft);
        stream <<  ssDefinition.at(ss);
    }
    catch(std::out_of_range) {qDebug() << "out of range exception (SS) num:" << ss;}
    stream.setFieldWidth(0);
    stream << "  ";
    int eeoid = getEOID();
    int addr = getAddress();
    QByteArray pcData = getPC21Data();
    switch(eeoid) {
        case 0x00:
            stream << "Physical Digits";
            break;
        case 0x01:
            stream << "Packed Physical Digits";
            if((addr>=1)&&(addr<=63)) stream << " (Digital inputs)";
            else if((addr>=65)&&(addr<=127)) stream << " (Door switch inputs)";
            else if(addr>=129) stream << " (Relay states)";
            stream << ", Start No:" << addr;
            if(pcData.count()) stream << " State:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2) stream << " Fault:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            if(pcData.count()>=3)stream << " Mask:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
            break;
        case 0x02:
            stream << "Deduced Digitals";
            break;
        case 0x03:
            stream << "Packed Deduced Digitals";
            if((addr>=0)&&(addr<=15)) stream << " (Local DD)";
            else if((addr>=16)&&(addr<=239)) stream << " (Cluster Global bits)";
            stream << ", Start No:" << addr+1;
            if(pcData.count()) stream << " State:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " Mask:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            break;
        case 0x04:
            stream << "Analogue Raw Data";
            break;
        case 0x05:
            stream << "Analogue Data Scaled";
            stream << ", No:" << addr;
            if(pcData.count()) stream << " Value:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " TDU:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            if(pcData.count()>=3)stream << " Alarm0:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
            if(pcData.count()>=4)stream << " Alarm1:" << ArchiveAnalyzer::getHexByte(pcData.at(3));
            if(pcData.count()>=5)stream << " Raw0:" << ArchiveAnalyzer::getHexByte(pcData.at(4));
            if(pcData.count()>=6)stream << " Raw1:" << ArchiveAnalyzer::getHexByte(pcData.at(5));
            break;
        case 0x06:
            stream << "Global Integer";
            if((addr>=1)&&(addr<=16)) stream << " (Local DA)";
            else if((addr>=17)&&(addr<=80)) stream << " (Cluster)";
            else if((addr>=81)&&(addr<=224)) stream << " (Network)";
            else if((addr>=240)&&(addr<=255)) stream << " (Key Number)";
            stream << ", No:" << addr;
            if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            break;
        case 0x07:
            stream << "Long Integers";
            break;
        case 0x08:
            stream << "Float Values";
            break;
        case 0x09:
            stream << "Heartbeat COUNT:" << addr;
            break;
        case 0x0A:
            stream << "Action Request";
            if(addr==0) stream << " (Display TX Mask),";
            else if(addr==1) stream << "(Telemetry TX Mask),";
            if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            if(pcData.count()>=3) {
                auto ioType = (quint8)pcData.at(2);
                stream << getIOType(ioType);
            }
            break;
        case 0x0B:
            stream << "Read Name";
            stream << ", Num:" << addr;
            if((ss==0)||(ss==1)) { // request
                if(pcData.count()) stream << " Point0:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                if(pcData.count()>=2) stream << " Point1:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                if(pcData.count()>=3) stream << getIOType((quint8)pcData.at(2));
                if(pcData.count()>=3) stream << " Node:" << ArchiveAnalyzer::getHexByte(pcData.at(3));
            }else if((ss==2)||(ss==3)) { // successful response
                if(pcData.count()) stream << " Message Num:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                if(pcData.count()>=2) {
                    stream << " Name:";
                    stream << (pcData.at(1)!='\0'?pcData.at(1):' ');
                    if(pcData.count()>=3) stream << (pcData.at(2)!='\0'?pcData.at(2):' ');
                    if(pcData.count()>=4) stream <<(pcData.at(3)!='\0'?pcData.at(3):' ');
                    if(pcData.count()>=5) stream << (pcData.at(4)!='\0'?pcData.at(4):' ');
                    if(pcData.count()>=6) stream << (pcData.at(5)!='\0'?pcData.at(5):' ');
                }
            }else { //unsuccessful response
                if(pcData.count()) stream << " Message Num:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            }
            break;
        case 0x0C:
            stream << "Cluster Status";
            switch(addr) {
                case 0:stream << " (Not Detected)";break;
                case 1:stream << " (Off Line)";break;
                case 2:stream << " (Incomplete)";break;
                case 3:stream << " (Invalid)";break;
                case 4:stream << " (Healthy)";break;
            }
            break;
        case 0x0D:
            stream << "Network Status";
            if(addr==0) stream << ", Not Networked";
            else if((addr>=1)&&(addr<=8)) stream << ", Cluster Num:" << addr;
            if(pcData.count()) stream << " State:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            break;
        case 0x0E:
            stream << "Network Packed Deduced Digitals";
            if((addr>=1)&&(addr<=128)) {
                quint8 cluster = (addr-1)>>4;
                stream << ", Cluster:"<<cluster;
            }
            stream << ", Start No:" << addr+256;
            if(pcData.count()) stream << " State:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " Mask:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            break;
        case 0x0F:
            stream << "Network Integer Values";
            if((addr>=1)&&(addr<=128)) {
                quint8 cluster = (addr-1)>>4;
                stream << ", Cluster:"<<cluster;
            }
            stream << ", No:" << addr+96;
            if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            break;
        case 0x10:
            stream << "Alarm Integer Values";
            if(addr==0) stream << ", Not Logged";
            else if(addr==1) stream << ", Logged";
            if(pcData.count()) stream << " Ref0:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2) stream << " Ref1:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            if(pcData.count()>=3) stream << " Value0:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
            if(pcData.count()>=4) stream << " Value1:" << ArchiveAnalyzer::getHexByte(pcData.at(3));
            break;
        case 0x11:
            stream << "Reader Message";
            stream << " ,Flag Byte:" << ArchiveAnalyzer::getHexByte(addr);
            if(addr&0x80) {
                stream << " (new with time)";
                if(pcData.count()) stream << " PID0:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                if(pcData.count()>=2) stream << " PID1:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                if(pcData.count()>=3) stream << " TIME0:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
                if(pcData.count()>=4) stream << " TIME1:" << ArchiveAnalyzer::getHexByte(pcData.at(3));
                if(pcData.count()>=5) stream << " TIME2:" << ArchiveAnalyzer::getHexByte(pcData.at(4));
                if(pcData.count()>=6) stream << " TIME3:" << ArchiveAnalyzer::getHexByte(pcData.at(5));
            }else {
                stream << " (old)";
                if(pcData.count()) stream << " HID0:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                if(pcData.count()>=2) stream << " HID1:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                if(pcData.count()>=3) stream << " PID0:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
                if(pcData.count()>=4) stream << " PID1:" << ArchiveAnalyzer::getHexByte(pcData.at(3));
            }
            break;
        case 0x12:
            stream << "Net Integer TDU Type Reference";
            if((addr>=1)&&(addr<=128)) {
                quint8 cluster = (addr-1)>>4;
                stream << ", Cluster:"<<cluster;
            }
            stream << ", No:" << addr+96;
            if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            break;
        case 0x1C:
            stream << "Node Command";
            if(addr==0) stream << " (Terminate)";
            else if(addr==1) stream << " (Restart/Boot)";
            else if(addr==2) stream << " (Refresh/Send all data)";
            break;
        case 0x1D:
            stream << "Telemetry controls";
            if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            break;
        case 0x1E:
            stream << "Telemetry Integers";
            stream << ", No:" << addr;
            if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
            if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
            if(pcData.count()>=3)stream << " Point Num:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
            break;
        case 0x1F:
            stream << "Module Data";
            switch(addr) {
                case 0:
                    stream << ", Module Type:";
                    if(pcData.count()) {
                        if(pcData.at(0)==1) stream << "PC21_1";
                        else if(pcData.at(0)==2) stream << "PC21_2D";
                        else if(pcData.at(0)==3) stream << "PC21_2T";
                    }
                    break;
                case 1:
                    stream << ", Boot Loader version:";
                    if(pcData.count()) stream << (pcData.at(0)!='\0'?pcData.at(0):' ');
                    if(pcData.count()>=2) stream << (pcData.at(1)!='\0'?pcData.at(1):' ');
                    if(pcData.count()>=3) stream << (pcData.at(2)!='\0'?pcData.at(2):' ');
                    if(pcData.count()>=4) stream << (pcData.at(3)!='\0'?pcData.at(3):' ');
                    if(pcData.count()>=5) stream << (pcData.at(4)!='\0'?pcData.at(4):' ');
                    break;
                case 2:
                    stream << ", OS version:";
                    if(pcData.count()) stream << (pcData.at(0)!='\0'?pcData.at(0):' ');
                    if(pcData.count()>=2) stream << (pcData.at(1)!='\0'?pcData.at(1):' ');
                    if(pcData.count()>=3) stream << (pcData.at(2)!='\0'?pcData.at(2):' ');
                    if(pcData.count()>=4) stream << (pcData.at(3)!='\0'?pcData.at(3):' ');
                    if(pcData.count()>=5) stream << (pcData.at(4)!='\0'?pcData.at(4):' ');
                    break;
                case 3:
                    stream << ", Application CN:";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    break;
                case 4:
                    stream << ", Analogue I/Ps Fitted";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " Next:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    if(pcData.count()>=3)stream << " Next:" << ArchiveAnalyzer::getHexByte(pcData.at(2));
                    if(pcData.count()>=4)stream << " Next:" << ArchiveAnalyzer::getHexByte(pcData.at(3));
                    break;
                case 5:
                    stream << ", Digital I/Ps Fitted";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    break;
                case 6:
                    stream << ", Switch I/Ps Fitted";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    break;
                case 7:
                    stream << ", Relay O/Ps Fitted";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    break;
                case 8:
                    stream << ", Analogue O/Ps Fitted";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    break;
                case 9:
                    stream << ", Internal Faults";
                    break;
                case 10:
                    stream << ", Relay Faults";
                    break;
                case 11:
                    stream << ", Analogue O/P Faults";
                    break;
                case 12:
                    stream << ", Telemetry Address:";
                    if(pcData.count()) stream << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    break;
                case 13:
                    stream << ", Telemetry Errors:";
                    if(pcData.count()) stream << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    break;
                case 14:
                    stream << ", Time and Date";
                    if(pcData.count()>=4) {
                        quint32 secs = 0;
                        secs = (quint8)pcData.at(3);
                        secs <<=8;secs|=(quint8)pcData.at(2);
                        secs <<=8;secs|=(quint8)pcData.at(1);
                        secs <<=8;secs|=(quint8)pcData.at(0);
                        QDateTime dt(QDate(2000,1,1),QTime(0,0,0));
                        dt = dt.addSecs(secs);
                        stream << dt.toString(" dd.MM.yyyy HH:mm:ss");
                    }
                    break;
                case 15:
                    stream << ", Communication Status:";
                    if(pcData.count()) stream << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    break;
                case 16:
                    stream << ", DHCP/IP";
                    break;
                case 17:
                    stream << ", EIP Communication Status:";
                    if(pcData.count()) {
                        if(pcData.at(0)==0) stream << " online";
                        else if(pcData.at(0)==1) stream << " offline";
                        else if(pcData.at(0)==2) stream << " SCADA off";
                    }
                    if(pcData.count()>=2) {
                        stream << " ,Speed:";
                        if(pcData.at(1)) stream << "100M";else stream << "10M";
                    }
                    if(pcData.count()>=3) {
                        stream << " ,Duplex:";
                        if(pcData.at(2)) stream << "Full";else stream << "Half";
                    }
                    break;
                case 18:
                    stream << ", Application Checksum";
                    if(pcData.count()) stream << " Low:" << ArchiveAnalyzer::getHexByte(pcData.at(0));
                    if(pcData.count()>=2)stream << " High:" << ArchiveAnalyzer::getHexByte(pcData.at(1));
                    break;
            }
            break;
    }

    return result;
}

QString CanRequest::getTime() const
{
    QDateTime dt(QDate(2000,1,1),QTime(0,0,0));
    dt = dt.addSecs(reqTime);
    return dt.toString("dd.MM.yyyy HH:mm:ss  ");
}


