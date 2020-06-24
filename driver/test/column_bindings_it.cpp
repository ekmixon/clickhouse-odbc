#include "driver/platform/platform.h"
#include "driver/test/client_utils.h"
#include "driver/test/client_test_base.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class ColumnBindingsTest
    : public ClientTestBase
{
};

class ColumnArrayBindingsTest
    : public ColumnBindingsTest
    , public ::testing::WithParamInterface<std::tuple<std::uintptr_t, std::size_t, std::size_t>>
{
};

template <typename CharType, std::size_t Size>
struct FixedStringBuffer {
    FixedStringBuffer() {
        // Fill with a special placeholder sequence.
        std::fill(data, data + Size, 0b11011011);
    }

    CharType data[Size];
};

TEST_P(ColumnArrayBindingsTest, ColumnWise) {
    const std::size_t total_rows_expected = std::get<1>(GetParam());
    const std::string query_orig = R"SQL(
SELECT
    CAST(number, 'Int32') AS col1,
    CAST(CAST(number, 'String'), 'FixedString(30)') AS col2,
    CAST(number, 'Float64') AS col3,
    CAST(if((number % 8) = 3, NULL, repeat('x', number % 41)), 'Nullable(String)') AS col4,
    CAST(number, 'UInt64') AS col5,
    CAST(number, 'Float32') AS col6
FROM numbers(
    )SQL" + std::to_string(total_rows_expected) + ")";

    const auto query = fromUTF8<SQLTCHAR>(query_orig);
    auto * query_wptr = const_cast<SQLTCHAR *>(query.c_str());

    ODBC_CALL_ON_STMT_THROW(hstmt, SQLPrepare(hstmt, query_wptr, SQL_NTS));
    ODBC_CALL_ON_STMT_THROW(hstmt, SQLExecute(hstmt));

    ODBC_CALL_ON_STMT_THROW(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)SQL_BIND_BY_COLUMN, 0));

    SQLULEN rows_processed = 0;
    ODBC_CALL_ON_STMT_THROW(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, (SQLPOINTER)&rows_processed, 0));

    const SQLULEN binding_offset = std::get<0>(GetParam());
    ODBC_CALL_ON_STMT_THROW(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_OFFSET_PTR, (SQLPOINTER)&binding_offset, 0));

    const std::size_t array_size = std::get<2>(GetParam());
    ODBC_CALL_ON_STMT_THROW(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)array_size, 0));

    const auto adjsut_ptr = [&] (auto * ptr) {
        return reinterpret_cast<char *>(ptr) - binding_offset;
    };

    const auto adjsut_ind_ptr = [&] (auto * ptr) {
        return reinterpret_cast<SQLLEN *>(adjsut_ptr(ptr));
    };

    std::vector<SQLINTEGER> col1(array_size);
    std::vector<SQLLEN> col1_ind(array_size);

    ODBC_CALL_ON_STMT_THROW(hstmt,
        SQLBindCol(
            hstmt,
            1,
            getCTypeFor<std::decay_t<decltype(col1[0])>>(),
            adjsut_ptr(&col1[0]),
            sizeof(col1[0]),
            adjsut_ind_ptr(&col1_ind[0])
        )
    );

    std::vector<FixedStringBuffer<SQLCHAR, 31>> col2(array_size);
    std::vector<SQLLEN> col2_ind(array_size);

    ODBC_CALL_ON_STMT_THROW(hstmt,
        SQLBindCol(
            hstmt,
            2,
            getCTypeFor<SQLCHAR *>(),
            adjsut_ptr(&col2[0]),
            sizeof(col2[0].data) * sizeof(col2[0].data[0]),
            adjsut_ind_ptr(&col2_ind[0])
        )
    );

    std::vector<SQLDOUBLE> col3(array_size);
    std::vector<SQLLEN> col3_ind(array_size);

    ODBC_CALL_ON_STMT_THROW(hstmt,
        SQLBindCol(
            hstmt,
            3,
            getCTypeFor<std::decay_t<decltype(col3[0])>>(),
            adjsut_ptr(&col3[0]),
            sizeof(col3[0]),
            adjsut_ind_ptr(&col3_ind[0])
        )
    );

    std::vector<FixedStringBuffer<SQLCHAR, 41>> col4(array_size);
    std::vector<SQLLEN> col4_ind(array_size);

    ODBC_CALL_ON_STMT_THROW(hstmt,
        SQLBindCol(
            hstmt,
            4,
            getCTypeFor<SQLCHAR *>(),
            adjsut_ptr(&col4[0]),
            sizeof(col4[0].data) * sizeof(col4[0].data[0]),
            adjsut_ind_ptr(&col4_ind[0])
        )
    );

    std::vector<SQLUBIGINT> col5(array_size);
    std::vector<SQLLEN> col5_ind(array_size);

    ODBC_CALL_ON_STMT_THROW(hstmt,
        SQLBindCol(
            hstmt,
            5,
            getCTypeFor<std::decay_t<decltype(col5[0])>>(),
            adjsut_ptr(&col5[0]),
            sizeof(col5[0]),
            adjsut_ind_ptr(&col5_ind[0])
        )
    );

    std::vector<SQLREAL> col6(array_size);
    std::vector<SQLLEN> col6_ind(array_size);

    ODBC_CALL_ON_STMT_THROW(hstmt,
        SQLBindCol(
            hstmt,
            6,
            getCTypeFor<std::decay_t<decltype(col6[0])>>(),
            adjsut_ptr(&col6[0]),
            sizeof(col6[0]),
            adjsut_ind_ptr(&col6_ind[0])
        )
    );

    std::size_t total_rows = 0;

    while (true) {
        std::fill(col1.begin(), col1.end(), -123);
        std::fill(col1_ind.begin(), col1_ind.end(), -123);

        std::fill(col2.begin(), col2.end(), FixedStringBuffer<SQLCHAR, 31>{});
        std::fill(col2_ind.begin(), col2_ind.end(), -123);

        std::fill(col3.begin(), col3.end(), -123);
        std::fill(col3_ind.begin(), col3_ind.end(), -123);

        std::fill(col4.begin(), col4.end(), FixedStringBuffer<SQLCHAR, 41>{});
        std::fill(col4_ind.begin(), col4_ind.end(), -123);

        std::fill(col5.begin(), col5.end(), 123456789);
        std::fill(col5_ind.begin(), col5_ind.end(), -123);

        std::fill(col6.begin(), col6.end(), -123);
        std::fill(col6_ind.begin(), col6_ind.end(), -123);

        rows_processed = 0;

        const SQLRETURN rc = SQLFetch(hstmt);

        if (rc == SQL_NO_DATA)
            break;

        if (rc == SQL_ERROR)
            throw std::runtime_error(extract_diagnostics(hstmt, SQL_HANDLE_STMT));

        if (rc == SQL_SUCCESS_WITH_INFO)
            std::cout << extract_diagnostics(hstmt, SQL_HANDLE_STMT) << std::endl;

        if (!SQL_SUCCEEDED(rc))
            throw std::runtime_error("SQLFetch return code: " + std::to_string(rc));

        ASSERT_LE(rows_processed, array_size);

        for (std::size_t i = 0; i < col1.size(); ++i) {
            if (i < rows_processed) {
                const std::int64_t number = total_rows + i;
                const auto number_str = std::to_string(number);

                EXPECT_EQ(col1[i], number);
                EXPECT_EQ(col1_ind[i], sizeof(col1[i]));

                {
                    auto number_str_col2 = number_str;
                    number_str_col2.resize(31, '\0'); // Because the column is 'FixedString(30)' + \0 from ODBC.

                    EXPECT_THAT(col2[i].data, ::testing::ElementsAreArray(number_str_col2));
                    EXPECT_EQ(col2_ind[i], 30); // Because the column is 'FixedString(30)'.
                }

                EXPECT_DOUBLE_EQ(col3[i], number);
                EXPECT_EQ(col3_ind[i], sizeof(col3[i]));

                if ((number % 8) == 3) {
                    EXPECT_THAT(col4[i].data, ::testing::Each(0b11011011));
                    EXPECT_EQ(col4_ind[i], SQL_NULL_DATA);
                }
                else {
                    std::string number_str_col4(number % 41, 'x');
                    number_str_col4.push_back('\0');
                    number_str_col4.resize(sizeof(col4[i].data), 0b11011011);

                    EXPECT_THAT(col4[i].data, ::testing::ElementsAreArray(number_str_col4));
                    EXPECT_EQ(col4_ind[i], number % 41);
                }

                EXPECT_EQ(col5[i], number);
                EXPECT_EQ(col5_ind[i], sizeof(col5[i]));

                EXPECT_FLOAT_EQ(col6[i], number);
                EXPECT_EQ(col6_ind[i], sizeof(col6[i]));
            }
            else {
                EXPECT_EQ(col1[i], -123);
                EXPECT_EQ(col1_ind[i], -123);

                EXPECT_THAT(col2[i].data, ::testing::Each(0b11011011));
                EXPECT_EQ(col2_ind[i], -123);

                EXPECT_DOUBLE_EQ(col3[i], -123);
                EXPECT_EQ(col3_ind[i], -123);

                EXPECT_THAT(col4[i].data, ::testing::Each(0b11011011));
                EXPECT_EQ(col4_ind[i], -123);

                EXPECT_EQ(col5[i], 123456789);
                EXPECT_EQ(col5_ind[i], -123);

                EXPECT_FLOAT_EQ(col6[i], -123);
                EXPECT_EQ(col6_ind[i], -123);
            }
        }

        total_rows += rows_processed;
    }

    ASSERT_EQ(total_rows, total_rows_expected);
}

INSTANTIATE_TEST_SUITE_P(ArrayBindings, ColumnArrayBindingsTest,
    ::testing::Combine(
        ::testing::Values(0, 1, 1234),                            // Binding offset.
        ::testing::Values(1, 2, 10, 75, 377, 4289),               // Result set sizes.
        ::testing::Values(1, 2, 3, 5, 10, 30, 47, 111, 500, 1000) // Row set sizes.
    ),
    [] (const auto & param_info) {
        return (
            "BindingOffset_" +std::to_string(std::get<0>(param_info.param))
            + "_vs_" +
            "ResultSet_" +std::to_string(std::get<1>(param_info.param))
            + "_vs_" +
            "RowSet_" +std::to_string(std::get<2>(param_info.param))
        );
    }
);
