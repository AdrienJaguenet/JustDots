#pragma once

#include "MoveMessage.h"
#include "JoinMessage.h"
#include "ACKJoinMessage.h"
#include "QuitMessage.h"
#include "CloseMessage.h"
#include "SetMessage.h"

typedef enum
{
	MSG_ACK = 0,
	MSG_JOIN = 1,
	MSG_MOVE = 2,
	MSG_QUIT = 3,
	MSG_ACKJOIN = 4,
	MSG_CLOSE = 5,
	MSG_SET = 6
} MessageType;

typedef struct
{
	MessageType type;
	union {
		JoinMessage join;
		MoveMessage move;
		QuitMessage quit;
		ACKJoinMessage ackjoin;
		CloseMessage close;
		SetMessage set;
	};
} BaseMessage;

