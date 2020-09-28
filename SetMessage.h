#pragma once

typedef struct {
	int exists;
	int x;
	int y;
} SetEntry;

typedef struct {
	SetEntry entries[10];
} SetMessage;

