#pragma once

#include "../common.h"

typedef char *ID;

/*
This hash is useful in our use case where we have english words, giving very low
chance of collision.
*/
uint64_t id_ASCIISumHash(ID str_id);

/*
TODO: Implement a python like dict system.
*/
