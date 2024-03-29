/*
  This file is a part of the
  Amagami SS PS2 Translation Project

  You may feel free to use this code if you want so.
  I provide no guarantee that this code is working,
  useful or anything else. Use it at your own risk.
*/

#ifndef DEFINES_H
#define DEFINES_H

#include <assert.h>
#include <stddef.h>

#include <QtGlobal>

#include "xmlhelpers.h"

/* Header */
const char header[] = "SCF";
const size_t HEADER_LENGTH = 3;

/* Labels section header  */
const char labelsSectionHeader[] = { 0x1A, 0x00, 0x04 };
const size_t LABELS_HEADER_LENGTH = 3;

/* Labels section header PAC */
const char labelsPSectionHeader[] = { 0x1A, 0x08, 0x04 };
const size_t LABELS_PHEADER_LENGTH = 3;

/* Empty section  */
const char emptySection[] = { 0x00, 0x00 };
const size_t EMPTY_SECTION_LENGTH = 2;

#define IS_ASCII_CODE(c) ((c) < 0x80)

/* Entry types */
enum
{
    ENTRY_TYPE_00 = 0x00,
    ENTRY_TYPE_01 = 0x01,
    ENTRY_TYPE_02 = 0x02,
    ENTRY_TYPE_03 = 0x03,
    ENTRY_TYPE_04 = 0x04,
    ENTRY_TYPE_05 = 0x05,
    ENTRY_TYPE_06 = 0x06,
    ENTRY_TYPE_07 = 0x07,
    ENTRY_TYPE_08 = 0x08
};

/* Languages */
typedef enum lang_id
{
    LANGUAGE_ENG,
    LANGUAGE_RUS,
    LANGUAGE_JAP,
    LANGUAGE_UNKNOWN
} lang_id;
#endif // DEFINES_H
