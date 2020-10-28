#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"


GBNRdtSender::GBNRdtSender() :nextSeqNum(0), waitingState(false), base(0)
{
}


GBNRdtSender::~GBNRdtSender()
{
}


bool GBNRdtSender::getWaitingState() {
	return waitingState;
}


bool GBNRdtSender::send(const Message& message) {
	if (!waitingState) {
		this->packetWaitingAck[nextSeqNum].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[nextSeqNum].seqnum = this->nextSeqNum;
		this->packetWaitingAck[nextSeqNum].checksum = 0;
		memcpy(this->packetWaitingAck[nextSeqNum].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[nextSeqNum].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[nextSeqNum]);
		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[nextSeqNum]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[nextSeqNum]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		if (base == nextSeqNum) {
			pns->startTimer(SENDER, Configuration::TIME_OUT, packetWaitingAck[nextSeqNum].seqnum);			//�������ͷ���ʱ��
		}
		nextSeqNum = (nextSeqNum + 1) % GBN_seqnum;  //8 = 2^k;
		printf("***************���ͱ��ĺ󣬵�ǰ���ڣ�[%d,%d]\t��base=%d��nextseqnum=%d *****************************\n", base,(base-1+GBN_Winsize)%GBN_seqnum, base, nextSeqNum);
		if ((nextSeqNum-base+ GBN_seqnum)% GBN_seqnum == GBN_Winsize) {
			waitingState = true;
		}
		//printf("waitingstate = %d\n", waitingState == false ? 0 : 1);
		return true;
	}
	else {
		return false;
	}
}

void GBNRdtSender::receive(const Packet& ackPkt) {
	//������ͷ����ڵȴ�ack��״̬�������´�������ʲô������
		//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ�������[base,base+N)������
	//��Ϊ���շ�ֻ����������ŵı���
	if (checkSum == ackPkt.checksum&& ((ackPkt.acknum-base>=0&& ackPkt.acknum - base <= GBN_Winsize)||(ackPkt.acknum+1+ GBN_seqnum -base<=GBN_Winsize))) {
		waitingState = false;
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		//base++;
		int buf = base;
		base = (ackPkt.acknum + 1)% GBN_seqnum;  //base����
		printf("***************��ǰ���ڣ�[%d,%d]\t��base=%d��nextseqnum=%d *****************************\n", base, (base - 1 + GBN_Winsize) % GBN_seqnum, base, nextSeqNum);
		if (base == nextSeqNum) {   //��ʱ����Ϊ0���رոôν��ܵ����кŶ�Ӧ�ļ�ʱ��
			pns->stopTimer(SENDER, buf);		//�رն�ʱ��������seqnum����һ��starttimer�Ĳ���
		}
		else {  //����������ʱ��������Ϊ��һ���������ܵ����к�
			pns->stopTimer(SENDER, buf);									//���ȹرն�ʱ��
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);			//�����������ͷ���ʱ��
			
		}
	}
	//else:���ͷ�û����ȷ�յ�ȷ�ϱ��ģ����� ,��������
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	printf("----------------���ͷ���ʱ�������ط������������[%d,%d)��Χ�ڵı���,��base=%d,seqnum=%d-----------------\n", base, nextSeqNum,base,nextSeqNum);
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	int len = (this->nextSeqNum + GBN_seqnum - this->base) % GBN_seqnum;   //�����·��͵ı�����Ŀ
	//�ط�С��nextseqnum֮ǰ�����б���
	//for (int j = base; j < nextSeqNum; j++) {
	for (int i = 0,j = base; i<len ; i++,j=(j+1)% GBN_seqnum) {
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", this->packetWaitingAck[j]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[j]);			//���·������ݰ�
	}
}
