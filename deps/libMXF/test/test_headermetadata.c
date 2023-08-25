#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>


static const mxfUUID someUUID =
    {0xff, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa};
static const mxfUL someUL =
    {0x06, 0x0a, 0x2b, 0x34, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa};
static const mxfTimestamp someTimestamp =
    {2006, 6, 9, 10, 10, 20, 10};
static const mxfUMID someUMID =
    {0xff, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa};
static const mxfRational someRational =
    {20, -10};

#define ITEM_DEF(count, typeId) \
    MXF_ITEM_DEFINITION(TestSet1, TestItem##count, \
        MXF_LABEL(count, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), \
        0x0000, \
        typeId, \
        0 \
    ); \

#define EXT_DATA_MODEL \
MXF_SET_DEFINITION(InterchangeObject, TestSet1, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01) \
); \
\
    ITEM_DEF(1, MXF_UINT8_TYPE) \
    ITEM_DEF(2, MXF_UINT16_TYPE) \
    ITEM_DEF(3, MXF_UINT32_TYPE) \
    ITEM_DEF(4, MXF_UINT64_TYPE) \
    ITEM_DEF(5, MXF_INT8_TYPE) \
    ITEM_DEF(6, MXF_INT16_TYPE) \
    ITEM_DEF(7, MXF_INT32_TYPE) \
    ITEM_DEF(8, MXF_INT64_TYPE) \
    ITEM_DEF(9, MXF_VERSIONTYPE_TYPE) \
    ITEM_DEF(10, MXF_UUID_TYPE) \
    ITEM_DEF(11, MXF_UL_TYPE) \
    ITEM_DEF(12, MXF_UMID_TYPE) \
    ITEM_DEF(13, MXF_TIMESTAMP_TYPE) \
    ITEM_DEF(14, MXF_UTF16STRING_TYPE) \
    ITEM_DEF(15, MXF_STRONGREF_TYPE) \
    ITEM_DEF(16, MXF_WEAKREF_TYPE) \
    ITEM_DEF(17, MXF_RATIONAL_TYPE) \
    ITEM_DEF(18, MXF_POSITION_TYPE) \
    ITEM_DEF(19, MXF_LENGTH_TYPE) \
    ITEM_DEF(20, MXF_BOOLEAN_TYPE) \
    ITEM_DEF(21, MXF_ULBATCH_TYPE) \
    ITEM_DEF(22, MXF_ULBATCH_TYPE) \
    ITEM_DEF(23, MXF_STRONGREFARRAY_TYPE) \
    ITEM_DEF(24, MXF_STRONGREFARRAY_TYPE) \
    ITEM_DEF(25, MXF_RAW_TYPE) \
    ITEM_DEF(26, MXF_PRODUCTVERSION_TYPE) \
    ITEM_DEF(27, MXF_UTF16STRING_TYPE) \
    ITEM_DEF(28, MXF_ULBATCH_TYPE) \
    ITEM_DEF(29, MXF_UTF8STRING_TYPE) \
    ITEM_DEF(30, MXF_ISO7STRING_TYPE) \
    ITEM_DEF(31, MXF_UUIDARRAY_TYPE) \
\
MXF_SET_DEFINITION(InterchangeObject, TestSet2, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02) \
); \
\
MXF_SET_DEFINITION(InterchangeObject, TestSet3, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03) \
); \
\
MXF_SET_DEFINITION(InterchangeObject, TestSet4, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04) \
); \
\
MXF_SET_DEFINITION(InterchangeObject, TestSet5, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05) \
); \
\
MXF_SET_DEFINITION(InterchangeObject, TestSet6, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06) \
); \
\
MXF_SET_DEFINITION(InterchangeObject, TestSet7, \
    MXF_LABEL(0x06, 0x0e, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07) \
);



#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;

#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

EXT_DATA_MODEL

#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION
#undef MXF_LABEL


int load_extensions(MXFDataModel *dataModel)
{
#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_ORET(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));

#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_ORET(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired));

EXT_DATA_MODEL

#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION

    return 1;
}

typedef struct
{
    int skippedBeforeCount;
    int skippedAfterCount;
    int nonSkippedBeforeCount;
    int nonSkippedAfterCount;
} FilterData;


static int before_set_read(void *privateData, MXFHeaderMetadata *headerMetadata,
                           const mxfKey *key, uint8_t llen, uint64_t len, int *skip)
{
    FilterData *filterData = (FilterData*)privateData;

    (void)headerMetadata;
    (void)llen;
    (void)len;

    /* TestSet1 is skipped */

    if (mxf_equals_key(key, &MXF_SET_K(Preface)))
    {
        filterData->nonSkippedBeforeCount++;
        *skip = 0;
    }
    else if (mxf_equals_key(key, &MXF_SET_K(TestSet1)))
    {
        filterData->skippedBeforeCount++;
        *skip = 1;
    }
    else
    {
        filterData->nonSkippedBeforeCount++;
        *skip = 0;
    }

    return 1;
}

static int after_set_read(void *privateData, MXFHeaderMetadata *headerMetadata, MXFMetadataSet *set, int *skip)
{
    FilterData *filterData = (FilterData*)privateData;

    (void)headerMetadata;

    /* All except Preface are skipped */

    if (mxf_equals_key(&set->key, &MXF_SET_K(Preface)))
    {
        filterData->nonSkippedAfterCount++;
        *skip = 0;
    }
    else
    {
        filterData->skippedAfterCount++;
        *skip = 1;
    }

    return 1;
}



int test_read(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFPartition *footerPartition = NULL;
    MXFDataModel *dataModel = NULL;
    MXFHeaderMetadata *headerMetadata = NULL;
    MXFDataModel *srcDataModel = NULL;
    MXFHeaderMetadata *srcHeaderMetadata = NULL;
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *set1;
    MXFMetadataSet *set2;
    MXFMetadataSet *set3;
    MXFMetadataSet *set4;
    MXFMetadataSet *set5;
    MXFMetadataSet *set6;
    MXFMetadataSet *set7;
    MXFMetadataSet *srcPrefaceSet;
    MXFMetadataSet *srcSet1;
    MXFMetadataSet *srcSet2;
    MXFMetadataSet *srcSet3;
    MXFMetadataSet *srcSet4;
    MXFMetadataSet *srcSet5;
    MXFMetadataSet *srcSet6;
    MXFMetadataSet *srcSet7;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint8_t value1;
    uint16_t value2;
    uint32_t value3;
    uint64_t value4;
    int8_t value5;
    int16_t value6;
    int32_t value7;
    int64_t value8;
    mxfVersionType value9;
    mxfUUID value10;
    mxfUL value11;
    mxfUMID value12;
    mxfTimestamp value13;
    uint16_t value14Size;
    mxfUTF16Char value14[128];
    mxfRational value17;
    mxfPosition value18;
    mxfLength value19;
    mxfBoolean value20;
    uint16_t value27Size;
    mxfUTF16Char value27[100];
    uint16_t value29Size;
    char value29[100];
    uint16_t value30Size;
    char value30[100];
    MXFArrayItemIterator arrayIter;
    uint32_t arrayCount;
    uint8_t *arrayElement;
    uint32_t arrayElementLength;
    mxfProductVersion value26;
    MXFMetadataSet *set;
    mxfUL ul;
    mxfUUID uuid;
    int64_t headerMetadataFilePos;
    MXFListIterator setsIter;
    FilterData filterData;
    MXFReadFilter readFilter;


    if (!mxf_disk_file_open_read(filename, &mxfFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);

    CHK_OFAIL(mxf_load_data_model(&dataModel));
    CHK_OFAIL(mxf_finalise_data_model(dataModel));
    CHK_OFAIL(mxf_create_header_metadata(&headerMetadata, dataModel));

    CHK_OFAIL(mxf_load_data_model(&srcDataModel));
    CHK_OFAIL(load_extensions(srcDataModel));
    CHK_OFAIL(mxf_finalise_data_model(srcDataModel));
    CHK_OFAIL(mxf_create_header_metadata(&srcHeaderMetadata, srcDataModel));


    /* TEST */

    /* read header pp */

    CHK_OFAIL(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &headerPartition));
    CHK_OFAIL(mxf_append_partition(&partitions, headerPartition));

    CHK_OFAIL((headerMetadataFilePos = mxf_file_tell(mxfFile)) >= 0);
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_header_metadata(&key));
    CHK_OFAIL(mxf_read_header_metadata(mxfFile, srcHeaderMetadata, headerPartition->headerByteCount, &key, llen, len));

    /* read and check items */

    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(Preface), &srcPrefaceSet));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet1), &srcSet1));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet2), &srcSet2));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet3), &srcSet3));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet4), &srcSet4));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet5), &srcSet5));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet6), &srcSet6));
    CHK_OFAIL(mxf_find_singular_set_by_key(srcHeaderMetadata, &MXF_SET_K(TestSet7), &srcSet7));

    /* test cloning by checking using a clone */
    CHK_OFAIL(mxf_clone_set(srcPrefaceSet, headerMetadata, &prefaceSet));
    CHK_OFAIL(mxf_clone_set(srcSet1, headerMetadata, &set1));
    CHK_OFAIL(mxf_clone_set(srcSet2, headerMetadata, &set2));
    CHK_OFAIL(mxf_clone_set(srcSet3, headerMetadata, &set3));
    CHK_OFAIL(mxf_clone_set(srcSet4, headerMetadata, &set4));
    CHK_OFAIL(mxf_clone_set(srcSet5, headerMetadata, &set5));
    CHK_OFAIL(mxf_clone_set(srcSet6, headerMetadata, &set6));
    CHK_OFAIL(mxf_clone_set(srcSet7, headerMetadata, &set7));

    CHK_OFAIL(mxf_get_uint8_item(set1, &MXF_ITEM_K(TestSet1, TestItem1), &value1));
    CHK_OFAIL(value1 == 0x0f);
    CHK_OFAIL(mxf_get_uint16_item(set1, &MXF_ITEM_K(TestSet1, TestItem2), &value2));
    CHK_OFAIL(value2 == 0x0f00);
    CHK_OFAIL(mxf_get_uint32_item(set1, &MXF_ITEM_K(TestSet1, TestItem3), &value3));
    CHK_OFAIL(value3 == 0x0f000000);
    CHK_OFAIL(mxf_get_uint64_item(set1, &MXF_ITEM_K(TestSet1, TestItem4), &value4));
    CHK_OFAIL(value4 == 0x0f00000000000000LL);
    CHK_OFAIL(mxf_get_int8_item(set1, &MXF_ITEM_K(TestSet1, TestItem5), &value5));
    CHK_OFAIL(value5 == -0x0f);
    CHK_OFAIL(mxf_get_int16_item(set1, &MXF_ITEM_K(TestSet1, TestItem6), &value6));
    CHK_OFAIL(value6 == -0x0f00);
    CHK_OFAIL(mxf_get_int32_item(set1, &MXF_ITEM_K(TestSet1, TestItem7), &value7));
    CHK_OFAIL(value7 == -0x0f000000);
    CHK_OFAIL(mxf_get_int64_item(set1, &MXF_ITEM_K(TestSet1, TestItem8), &value8));
    CHK_OFAIL(value8 == -0x0f00000000000000LL);
    CHK_OFAIL(mxf_get_version_type_item(set1, &MXF_ITEM_K(TestSet1, TestItem9), &value9));
    CHK_OFAIL(value9 == 256);
    CHK_OFAIL(mxf_get_uuid_item(set1, &MXF_ITEM_K(TestSet1, TestItem10), &value10));
    CHK_OFAIL(memcmp(&value10, &someUUID, sizeof(mxfUUID)) == 0);
    CHK_OFAIL(mxf_get_ul_item(set1, &MXF_ITEM_K(TestSet1, TestItem11), &value11));
    CHK_OFAIL(memcmp(&value11, &someUL, sizeof(mxfUL)) == 0);
    CHK_OFAIL(mxf_get_umid_item(set1, &MXF_ITEM_K(TestSet1, TestItem12), &value12));
    CHK_OFAIL(memcmp(&value12, &someUMID, sizeof(mxfUMID)) == 0);
    CHK_OFAIL(mxf_get_timestamp_item(set1, &MXF_ITEM_K(TestSet1, TestItem13), &value13));
    CHK_OFAIL(memcmp(&value13, &someTimestamp, sizeof(mxfTimestamp)) == 0);
    CHK_OFAIL(mxf_get_utf16string_item_size(set1, &MXF_ITEM_K(TestSet1, TestItem14), &value14Size));
    CHK_OFAIL(value14Size == wcslen(L"A UTF16 String") + 1);
    CHK_OFAIL(mxf_get_utf16string_item(set1, &MXF_ITEM_K(TestSet1, TestItem14), value14));
    CHK_OFAIL(wcscmp(L"A UTF16 String", value14) == 0);
    CHK_OFAIL(mxf_get_strongref_item(set1, &MXF_ITEM_K(TestSet1, TestItem15), &set2));
    CHK_OFAIL(memcmp(&set2->key, &MXF_SET_K(TestSet2), sizeof(mxfKey)) == 0);
    /* CHK_OFAIL(mxf_get_weakref_item(set1, &MXF_ITEM_K(TestSet1, TestItem16), &set3)); */
    /* cloning weak references is not yet supported, so we test the source set instead */
    CHK_OFAIL(mxf_get_weakref_item(srcSet1, &MXF_ITEM_K(TestSet1, TestItem16), &set3));
    CHK_OFAIL(memcmp(&set3->key, &MXF_SET_K(TestSet3), sizeof(mxfKey)) == 0);
    CHK_OFAIL(mxf_get_rational_item(set1, &MXF_ITEM_K(TestSet1, TestItem17), &value17));
    CHK_OFAIL(memcmp(&value17, &someRational, sizeof(mxfRational)) == 0);
    CHK_OFAIL(mxf_get_position_item(set1, &MXF_ITEM_K(TestSet1, TestItem18), &value18));
    CHK_OFAIL(value18 == 0x0f000000);
    CHK_OFAIL(mxf_get_length_item(set1, &MXF_ITEM_K(TestSet1, TestItem19), &value19));
    CHK_OFAIL(value19 == 0x0f000000);
    CHK_OFAIL(mxf_get_boolean_item(set1, &MXF_ITEM_K(TestSet1, TestItem20), &value20));
    CHK_OFAIL(value20 == 1);
    CHK_OFAIL(mxf_get_item_len(set1, &MXF_ITEM_K(TestSet1, TestItem27), &value27Size));
    CHK_OFAIL(value27Size == 100 * mxfUTF16Char_extlen);
    CHK_OFAIL(mxf_get_utf16string_item_size(set1, &MXF_ITEM_K(TestSet1, TestItem27), &value27Size));
    CHK_OFAIL(value27Size == wcslen(L"A fixed size UTF16 String") + 1);
    CHK_OFAIL(mxf_get_utf16string_item(set1, &MXF_ITEM_K(TestSet1, TestItem27), value27));
    CHK_OFAIL(wcscmp(L"A fixed size UTF16 String", value27) == 0);
    CHK_OFAIL(mxf_get_utf8string_item_size(set1, &MXF_ITEM_K(TestSet1, TestItem29), &value29Size));
    CHK_OFAIL(value29Size == strlen("An arbitrary UTF8 String") + 1);
    CHK_OFAIL(mxf_get_utf8string_item(set1, &MXF_ITEM_K(TestSet1, TestItem29), value29));
    CHK_OFAIL(strcmp("An arbitrary UTF8 String", value29) == 0);
    CHK_OFAIL(mxf_get_iso7string_item_size(set1, &MXF_ITEM_K(TestSet1, TestItem30), &value30Size));
    CHK_OFAIL(value30Size == strlen("An arbitrary ISO7 String") + 1);
    CHK_OFAIL(mxf_get_iso7string_item(set1, &MXF_ITEM_K(TestSet1, TestItem30), value30));
    CHK_OFAIL(strcmp("An arbitrary ISO7 String", value30) == 0);

    CHK_OFAIL(mxf_get_array_item_count(set1, &MXF_ITEM_K(TestSet1, TestItem21), &arrayCount));
    CHK_OFAIL(arrayCount == 0);

    CHK_OFAIL(mxf_get_array_item_count(set1, &MXF_ITEM_K(TestSet1, TestItem22), &arrayCount));
    CHK_OFAIL(arrayCount == 2);
    CHK_OFAIL(mxf_initialise_array_item_iterator(set1, &MXF_ITEM_K(TestSet1, TestItem22), &arrayIter));
    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLength))
    {
        mxf_get_ul(arrayElement, &ul);
        CHK_OFAIL(memcmp(&someUL, &ul, sizeof(mxfUL)) == 0);
    }

    CHK_OFAIL(mxf_get_array_item_count(set1, &MXF_ITEM_K(TestSet1, TestItem23), &arrayCount));
    CHK_OFAIL(arrayCount == 2);
    CHK_OFAIL(mxf_initialise_array_item_iterator(set1, &MXF_ITEM_K(TestSet1, TestItem23), &arrayIter));
    {
        CHK_OFAIL(mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLength));
        CHK_OFAIL(mxf_get_strongref(set1->headerMetadata, arrayElement, &set));
        CHK_OFAIL(memcmp(&set->key, &MXF_SET_K(TestSet4), sizeof(mxfKey)) == 0);

        CHK_OFAIL(mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLength));
        CHK_OFAIL(mxf_get_strongref(set1->headerMetadata, arrayElement, &set));
        CHK_OFAIL(memcmp(&set->key, &MXF_SET_K(TestSet5), sizeof(mxfKey)) == 0);
    }

    CHK_OFAIL(mxf_get_array_item_count(set1, &MXF_ITEM_K(TestSet1, TestItem24), &arrayCount));
    CHK_OFAIL(arrayCount == 2);
    CHK_OFAIL(mxf_get_array_item_element_len(set1, &MXF_ITEM_K(TestSet1, TestItem24), &arrayElementLength));
    CHK_OFAIL(arrayElementLength == mxfUUID_extlen);
    CHK_OFAIL(mxf_get_array_item_element(set1, &MXF_ITEM_K(TestSet1, TestItem24), 0, &arrayElement));
    CHK_OFAIL(mxf_get_strongref(set1->headerMetadata, arrayElement, &set));
    CHK_OFAIL(memcmp(&set->key, &MXF_SET_K(TestSet6), sizeof(mxfKey)) == 0);
    CHK_OFAIL(mxf_get_array_item_element(set1, &MXF_ITEM_K(TestSet1, TestItem24), 1, &arrayElement));
    CHK_OFAIL(mxf_get_strongref(set1->headerMetadata, arrayElement, &set));
    CHK_OFAIL(memcmp(&set->key, &MXF_SET_K(TestSet7), sizeof(mxfKey)) == 0);

    CHK_OFAIL(mxf_get_product_version_item(set1, &MXF_ITEM_K(TestSet1, TestItem26), &value26));
    CHK_OFAIL(memcmp(mxf_get_version(), &value26, sizeof(mxfProductVersion)) == 0);

    CHK_OFAIL(mxf_get_array_item_count(set1, &MXF_ITEM_K(TestSet1, TestItem28), &arrayCount));
    CHK_OFAIL(arrayCount == 2);
    CHK_OFAIL(mxf_initialise_array_item_iterator(set1, &MXF_ITEM_K(TestSet1, TestItem28), &arrayIter));
    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLength))
    {
        mxf_get_ul(arrayElement, &ul);
        CHK_OFAIL(memcmp(&someUL, &ul, sizeof(mxfUL)) == 0);
    }

    CHK_OFAIL(mxf_get_array_item_count(set1, &MXF_ITEM_K(TestSet1, TestItem31), &arrayCount));
    CHK_OFAIL(arrayCount == 2);
    CHK_OFAIL(mxf_initialise_array_item_iterator(set1, &MXF_ITEM_K(TestSet1, TestItem31), &arrayIter));
    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLength))
    {
        mxf_get_uuid(arrayElement, &uuid);
        CHK_OFAIL(memcmp(&someUUID, &uuid, sizeof(mxfUUID)) == 0);
    }


    /* test using the speedy dereferencing */
    mxf_initialise_sets_iter(headerMetadata, &setsIter);
    CHK_OFAIL(mxf_dereference_s(headerMetadata, &setsIter, &prefaceSet->instanceUID, &prefaceSet));
    CHK_OFAIL(mxf_dereference_s(headerMetadata, &setsIter, &prefaceSet->instanceUID, &prefaceSet));
    CHK_OFAIL(mxf_next_list_iter_element(&setsIter)); /* move it past the Preface */
    CHK_OFAIL(mxf_dereference_s(headerMetadata, &setsIter, &prefaceSet->instanceUID, &prefaceSet));


    /* test reading using filter */

    memset(&filterData, 0, sizeof(FilterData));
    readFilter.privateData = &filterData;
    readFilter.before_set_read = before_set_read;
    readFilter.after_set_read = after_set_read;

    /* read header metadata again, but now with the filter */
    mxf_free_header_metadata(&headerMetadata);
    CHK_OFAIL(mxf_create_header_metadata(&headerMetadata, dataModel));
    CHK_OFAIL(mxf_file_seek(mxfFile, headerMetadataFilePos, SEEK_SET));
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_header_metadata(&key));
    CHK_OFAIL(mxf_read_filtered_header_metadata(mxfFile, &readFilter, headerMetadata, headerPartition->headerByteCount,
                                                &key, llen, len));

    CHK_OFAIL(filterData.skippedBeforeCount == 1); /* TestSet1 skipped */
    CHK_OFAIL(filterData.nonSkippedBeforeCount == 7); /* all except TestSet1 */
    CHK_OFAIL(filterData.skippedAfterCount == 6); /* all except Preface */
    CHK_OFAIL(filterData.nonSkippedAfterCount == 1); /* Preface was not skipped */
    CHK_OFAIL(mxf_get_list_length(&headerMetadata->sets) == 1); /* Preface */




    /* skip filler and read footer pp */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &footerPartition));
    CHK_OFAIL(mxf_append_partition(&partitions, footerPartition));


    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_free_data_model(&dataModel);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&srcDataModel);
    mxf_free_header_metadata(&srcHeaderMetadata);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_free_data_model(&dataModel);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&srcDataModel);
    mxf_free_header_metadata(&srcHeaderMetadata);
    return 0;
}

int test_create_and_write(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFPartition *footerPartition = NULL;
    MXFDataModel *dataModel = NULL;
    MXFHeaderMetadata *headerMetadata = NULL;
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *set1;
    MXFMetadataSet *set2;
    MXFMetadataSet *set3;
    MXFMetadataSet *set4;
    MXFMetadataSet *set5;
    MXFMetadataSet *set6;
    MXFMetadataSet *set7;
    uint8_t *arrayElement;


    if (!mxf_disk_file_open_new(filename, &mxfFile))
    {
        mxf_log_error("Failed to create '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);

    CHK_OFAIL(mxf_load_data_model(&dataModel));
    CHK_OFAIL(load_extensions(dataModel));
    CHK_OFAIL(mxf_finalise_data_model(dataModel));

    CHK_OFAIL(mxf_create_header_metadata(&headerMetadata, dataModel));


    /* TEST */

    /* create header metadata */
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet1), &set1));
    mxf_set_fixed_set_space_allocation(set1, 2048);
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet2), &set2));
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet3), &set3));
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet4), &set4));
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet5), &set5));
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet6), &set6));
    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(TestSet7), &set7));

    CHK_OFAIL(mxf_set_uint8_item(set1, &MXF_ITEM_K(TestSet1, TestItem1), 0x0f));
    CHK_OFAIL(mxf_set_uint16_item(set1, &MXF_ITEM_K(TestSet1, TestItem2), 0x0f00));
    CHK_OFAIL(mxf_set_uint32_item(set1, &MXF_ITEM_K(TestSet1, TestItem3), 0x0f000000));
    CHK_OFAIL(mxf_set_uint64_item(set1, &MXF_ITEM_K(TestSet1, TestItem4), 0x0f00000000000000LL));
    CHK_OFAIL(mxf_set_int8_item(set1, &MXF_ITEM_K(TestSet1, TestItem5), -0x0f));
    CHK_OFAIL(mxf_set_int16_item(set1, &MXF_ITEM_K(TestSet1, TestItem6), -0x0f00));
    CHK_OFAIL(mxf_set_int32_item(set1, &MXF_ITEM_K(TestSet1, TestItem7), -0x0f000000));
    CHK_OFAIL(mxf_set_int64_item(set1, &MXF_ITEM_K(TestSet1, TestItem8), -0x0f00000000000000LL));
    CHK_OFAIL(mxf_set_version_type_item(set1, &MXF_ITEM_K(TestSet1, TestItem9), 256));
    CHK_OFAIL(mxf_set_uuid_item(set1, &MXF_ITEM_K(TestSet1, TestItem10), &someUUID));
    CHK_OFAIL(mxf_set_ul_item(set1, &MXF_ITEM_K(TestSet1, TestItem11), &someUL));
    CHK_OFAIL(mxf_set_umid_item(set1, &MXF_ITEM_K(TestSet1, TestItem12), &someUMID));
    CHK_OFAIL(mxf_set_timestamp_item(set1, &MXF_ITEM_K(TestSet1, TestItem13), &someTimestamp));
    CHK_OFAIL(mxf_set_utf16string_item(set1, &MXF_ITEM_K(TestSet1, TestItem14), L"A UTF16 String"));
    CHK_OFAIL(mxf_set_strongref_item(set1, &MXF_ITEM_K(TestSet1, TestItem15), set2));
    CHK_OFAIL(mxf_set_weakref_item(set1, &MXF_ITEM_K(TestSet1, TestItem16), set3));
    CHK_OFAIL(mxf_set_rational_item(set1, &MXF_ITEM_K(TestSet1, TestItem17), &someRational));
    CHK_OFAIL(mxf_set_position_item(set1, &MXF_ITEM_K(TestSet1, TestItem18), 0x0f000000));
    CHK_OFAIL(mxf_set_length_item(set1, &MXF_ITEM_K(TestSet1, TestItem19), 0x0f000000));
    CHK_OFAIL(mxf_set_boolean_item(set1, &MXF_ITEM_K(TestSet1, TestItem20), 1));
    CHK_OFAIL(mxf_set_fixed_size_utf16string_item(set1, &MXF_ITEM_K(TestSet1, TestItem27), L"A fixed size UTF16 String", 100));
    CHK_OFAIL(mxf_set_utf8string_item(set1, &MXF_ITEM_K(TestSet1, TestItem29), "An arbitrary UTF8 String"));
    CHK_OFAIL(mxf_set_iso7string_item(set1, &MXF_ITEM_K(TestSet1, TestItem30), "An arbitrary ISO7 String"));

    CHK_OFAIL(mxf_set_empty_array_item(set1, &MXF_ITEM_K(TestSet1, TestItem21), mxfUL_extlen));
    CHK_OFAIL(mxf_alloc_array_item_elements(set1, &MXF_ITEM_K(TestSet1, TestItem21), mxfUL_extlen, 0, &arrayElement));
    CHK_OFAIL(mxf_alloc_array_item_elements(set1, &MXF_ITEM_K(TestSet1, TestItem22), mxfUL_extlen, 2, &arrayElement));
    mxf_set_ul(&someUL, arrayElement);
    mxf_set_ul(&someUL, &arrayElement[mxfUL_extlen]);
    CHK_OFAIL(mxf_add_array_item_strongref(set1, &MXF_ITEM_K(TestSet1, TestItem23), set4));
    CHK_OFAIL(mxf_add_array_item_strongref(set1, &MXF_ITEM_K(TestSet1, TestItem23), set5));
    CHK_OFAIL(mxf_add_array_item_strongref(set1, &MXF_ITEM_K(TestSet1, TestItem24), set6));
    CHK_OFAIL(mxf_add_array_item_strongref(set1, &MXF_ITEM_K(TestSet1, TestItem24), set7));
    CHK_OFAIL(mxf_set_product_version_item(set1, &MXF_ITEM_K(TestSet1, TestItem26), mxf_get_version()));
    CHK_OFAIL(mxf_grow_array_item(set1, &MXF_ITEM_K(TestSet1, TestItem28), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&someUL, arrayElement);
    CHK_OFAIL(mxf_grow_array_item(set1, &MXF_ITEM_K(TestSet1, TestItem28), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&someUL, arrayElement);
    CHK_OFAIL(mxf_grow_array_item(set1, &MXF_ITEM_K(TestSet1, TestItem31), mxfUUID_extlen, 1, &arrayElement));
    mxf_set_uuid(&someUUID, arrayElement);
    CHK_OFAIL(mxf_grow_array_item(set1, &MXF_ITEM_K(TestSet1, TestItem31), mxfUUID_extlen, 1, &arrayElement));
    mxf_set_uuid(&someUUID, arrayElement);

    /* write the header pp */
    CHK_OFAIL(mxf_append_new_partition(&partitions, &headerPartition));
    headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    CHK_OFAIL(mxf_write_partition(mxfFile, headerPartition));

    /* write header metadata */
    CHK_OFAIL(mxf_mark_header_start(mxfFile, headerPartition));
    CHK_OFAIL(mxf_write_header_metadata(mxfFile, headerMetadata));
    CHK_OFAIL(mxf_mark_header_end(mxfFile, headerPartition));


    /* write the footer pp */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition,
        &footerPartition));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    CHK_OFAIL(mxf_write_partition(mxfFile, footerPartition));

    /* update the partitions */
    CHK_OFAIL(mxf_update_partitions(mxfFile, &partitions));


    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_free_data_model(&dataModel);
    mxf_free_header_metadata(&headerMetadata);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_free_data_model(&dataModel);
    mxf_free_header_metadata(&headerMetadata);
    return 0;
}


void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s filename\n", cmd);
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    /* replace util functions to ensure files are identical */
    mxf_set_regtest_funcs();
    mxf_avid_set_regtest_funcs();


    if (!test_create_and_write(argv[1]))
    {
        return 1;
    }

    if (!test_read(argv[1]))
    {
        return 1;
    }

    return 0;
}

