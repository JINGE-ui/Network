#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#define TCP_Winsize 8
#define TCP_seqnum 16  

#define GBN_Winsize 4
#define GBN_seqnum 8   //2^3 = 8

#define SR_Winsize 4
#define SR_seqnum 8

struct  Configuration{

	/**
	�������Э��Payload���ݵĴ�С���ֽ�Ϊ��λ��
	*/
	static const int PAYLOAD_SIZE = 21;

	/**
	��ʱ��ʱ��
	*/
	static const int TIME_OUT =20;

};



/**
	�����Ӧ�ò����Ϣ
*/
struct  Message {
	char data[Configuration::PAYLOAD_SIZE];		//payload

	Message();
	Message(const Message &msg);
	virtual Message & operator=(const Message &msg);
	virtual ~Message();

	virtual void print();
};

/**
	���Ĳ�����㱨�Ķ�
*/
struct  Packet {
	int seqnum;										//���
	int acknum;										//ȷ�Ϻ�
	int checksum;									//У���
	char payload[Configuration::PAYLOAD_SIZE];		//payload
	
	Packet();
	Packet(const Packet& pkt);
	virtual Packet & operator=(const Packet& pkt);
	virtual bool operator==(const Packet& pkt) const;
	virtual ~Packet();

	virtual void print();
};



#endif

