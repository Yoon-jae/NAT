#include "StdAfx.h"
#include "EthernetLayer.h"

CEthernetLayer::CEthernetLayer(char* pName) : CBaseLayer(pName)
{
	ResetHeader();
}

CEthernetLayer::~CEthernetLayer()
{
}

// Header를 초기화 하는 함수.
void CEthernetLayer::ResetHeader()
{

	memset(Ethernet_Header.Ethernet_srcAddr.addr_ethernet, 0, 6); // destination address initialize
	memset(Ethernet_Header.Ethernet_dstAddr.addr_ethernet, 0, 6); // source address initialize
	Ethernet_Header.Ethernet_type = 0x0800; // data type initialize
	memset(Ethernet_Header.Ethernet_data, 0, ETHERNET_MAX_DATA); // data initialize
}

// 송신자의 ethernet 주소를 얻어오는 함수.
unsigned char* CEthernetLayer::GetSourceAddress(int dev_num)
{
	if(dev_num == 1)
		return this->dev_1_mac_addr;
	return this->dev_2_mac_addr;
	// 소스 주소를 얻는 함수이다.
	// 멤버변수 m_sHeader의 소스주소가 담긴 .enet_srcaddr을 리턴한다.
}

unsigned char* CEthernetLayer::GetSourceAddressForRip(int dev_num)
{
	if(dev_num == 1)
		return dev_1_mac_addr_for_rip;
	return dev_2_mac_addr_for_rip;
}



// 수신자의 ethernet 주소를 얻어오는 함수.
unsigned char* CEthernetLayer::GetDestinAddress()
{	
	return Ethernet_Header.Ethernet_dstAddr.addr_ethernet;
	//목적지 주소를 얻는 함수 이다.
	// 맴버변수 m_sHeader의 목적지주소가 담긴 .enet_dstaddr를 리턴한다.
}

// 송시자의 ethernet 주소를 설정하는 함수.
void CEthernetLayer::SetSourceAddress(unsigned char *pAddress, int dev_num)
{
	// pAddress로 넘어온 인자를 memcpy 한다.
	if(dev_num == 1)
		memcpy(dev_1_mac_addr,pAddress, 6);
	else
		memcpy(dev_2_mac_addr,pAddress, 6);

	memcpy( Ethernet_Header.Ethernet_srcAddr.addr_ethernet, pAddress, 6);
}

void CEthernetLayer::SetSourceAddressForRip(unsigned char* pAddress, int dev_num)
{
	if(dev_num == 1)
		memcpy(dev_1_mac_addr_for_rip , pAddress , 4 );
	else 
		memcpy(dev_2_mac_addr_for_rip , pAddress , 4 );
}

// 수신자의 ethernet 주소를 설정하는 함수.
void CEthernetLayer::SetDestinAddress(unsigned char *pAddress, int dev_num)
{
	if(dev_num == 1)
		memcpy(dev_1_dst_mac_addr,pAddress,6);
	else
		memcpy(dev_2_dst_mac_addr,pAddress,6);

	memcpy( Ethernet_Header.Ethernet_dstAddr.addr_ethernet, pAddress, 6) ;
	// 맴버 변수의 목적지 주소를 넣는 enet_dstaddr에 
	//인자로 받은 paddress의 값을 복사한다.
}

// 그래서 매개변수로 넘어오는 ppayload의 내용은 test packet에 입력한 "Group 6 test packet" 이라는 문자열 입니다.
BOOL CEthernetLayer::Send(unsigned char *ppayload, int nlength, int dev_num)
{
	memcpy(Ethernet_Header.Ethernet_data, ppayload, nlength);
	BOOL bSuccess=mp_UnderLayer->Send((unsigned char*) &Ethernet_Header, nlength + ETHERNET_HEADER_SIZE, dev_num);
	// NILayer에 data를 보내는 부분.
	return bSuccess ;
}

// NILayer에서 data를 얻어오고 demultiplexing 하고 상위 계층으로 보내주는 형식으로 구현 하면 될것같습니다.
BOOL CEthernetLayer::Receive(unsigned char* ppayload, int dev_num)
{
	PEthernetHeader pFrame = (PEthernetHeader) ppayload;
	char Broad[6];
	memset(Broad,0xff,6);

	char IPv4mcast_mask[6] = {0x01,0x00,0x5e,0x80,0x00,0x00};
	char IPv4mcast[6] = {0x01,0x00,0x5e,0x00,0x00,0x00};

	char result[6];
	for(int i=0; i<6; i++) {
		result[i] = pFrame->Ethernet_dstAddr.S_un.s_ether_addr[i] & IPv4mcast_mask[i];
	}

	EthernetHeader Ethernet_Header;

	if(!memcmp(&pFrame->Ethernet_srcAddr,GetSourceAddress(dev_num),6)) //자신에게 보낸 페킷
		return FALSE;
	
	if((!memcmp(&pFrame->Ethernet_dstAddr,GetSourceAddress(dev_num),6)) ||
	   (!memcmp(&pFrame->Ethernet_dstAddr,Broad,6)) ||
		!memcmp(result,IPv4mcast,6)) {
		//Broad Cast or 자신 Mac주소 or Multicast
		if(pFrame->Ethernet_type == arp_type) //arp_type일 경우
			GetUpperLayer(1)->Receive((unsigned char*) pFrame->Ethernet_data, dev_num);
		else if(pFrame->Ethernet_type == ip_type) { //ip_type일 경우
			SetSourceAddressForRip((unsigned char*) &pFrame->Ethernet_srcAddr,dev_num);
			GetUpperLayer(0)->Receive((unsigned char*) pFrame->Ethernet_data, dev_num);
		}
	}

	return FALSE;
}

void CEthernetLayer::SetType(unsigned short type)
{
	Ethernet_Header.Ethernet_type = type;
}

BOOL CEthernetLayer::Send(unsigned char* ppayload, int nlength, unsigned short type, int dev_num)
{
	char Broad[6];
	memset(Broad,0xff,6);

	if(dev_num == 1)
		memcpy(&Ethernet_Header.Ethernet_srcAddr,dev_1_mac_addr, 6);
	else
		memcpy(&Ethernet_Header.Ethernet_srcAddr,dev_2_mac_addr, 6);

	memcpy(&Ethernet_Header.Ethernet_data, ppayload, nlength); // data 부분을 복사 한다.
	Ethernet_Header.Ethernet_type = type;

	// NILayer에 data를 보내는 부분.
	return GetUnderLayer()->Send((unsigned char *) &Ethernet_Header,nlength + ETHERNET_HEADER_SIZE, dev_num);
}