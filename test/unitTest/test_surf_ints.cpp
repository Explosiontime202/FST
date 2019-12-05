#include "gtest/gtest.h"

#include <assert.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "surf.hpp"
#include <cstdlib>
#include <chrono>

namespace surf {

    namespace surftest {

        static const SuffixType kSuffixType = kReal;
        static const level_t kSuffixLen = 8;
        static const uint32_t number_keys = 250000;
        static const uint32_t kIntTestSkip = 9;

        class SuRFInt32Test : public ::testing::Test {
        public:
            void SetUp() override
            {
                // initialize keys and values vectors
                keys_int32.resize(number_keys);
                values_uint64.resize(number_keys);

                // generate keys and values
                uint32_t value = 3;
                for (uint32_t i = 0; i < number_keys; i++) {
                    keys_int32[i] = uint32ToString(value);
                    value += kIntTestSkip;
                    values_uint64[i] = i;
                }
                std::random_shuffle(values_uint64.begin(), values_uint64.end());

                std::cout << "number keys: " << std::to_string(keys_int32.size() / 1000000) << "M" << std::endl;
            }

            void TearDown() override {}

            std::vector<std::string> keys_int32;
            std::vector<uint64_t> values_uint64;
        };


        TEST_F (SuRFInt32Test, PointLookupTests) {
            auto start = std::chrono::high_resolution_clock::now();
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;
            std::cout << "build time " << std::to_string(elapsed.count()) << std::endl;

            start = std::chrono::high_resolution_clock::now();
            for (uint64_t i = 0; i < values_uint64.size(); i++) {
                uint64_t retrieved_value = 0;
                bool exist = surf->lookupKey(keys_int32[i], retrieved_value);
                ASSERT_TRUE(exist);
                ASSERT_EQ(values_uint64[i], retrieved_value);
            }
            finish = std::chrono::high_resolution_clock::now();
            elapsed = finish - start;
            std::cout << "query time " << std::to_string(elapsed.count()) << std::endl;


        }

        TEST_F (SuRFInt32Test, IteratorTestsGreaterThanExclusive) {
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            size_t start_position = 7234;
            SuRF::Iter iter = surf->moveToKeyGreaterThan(keys_int32[start_position - 1], false);
            for (; start_position < keys_int32.size(); start_position++) {
                ASSERT_TRUE(iter.isValid());
                ASSERT_EQ(keys_int32[start_position].compare( iter.getKey()), 0);
                iter++;
            }
            ASSERT_FALSE(iter.isValid());
        }

        TEST_F (SuRFInt32Test, IteratorTestsGreaterThanInclusive) {
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            size_t start_position = 7234;
            SuRF::Iter iter = surf->moveToKeyGreaterThan(keys_int32[start_position], true);
            for (; start_position < keys_int32.size(); start_position++) {
                ASSERT_TRUE(iter.isValid());
                ASSERT_EQ(keys_int32[start_position].compare( iter.getKey()), 0);
                iter++;
            }
            ASSERT_FALSE(iter.isValid());
        }

        TEST_F (SuRFInt32Test, IteratorTestsRangeLookup) {
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            size_t start_position = 7234;
            size_t end_position = 7235;
            auto iterators = surf->lookupRange(keys_int32[start_position - 1], false, keys_int32[end_position], false);

            while (iterators.first != iterators.second) {
                ASSERT_TRUE(iterators.first.isValid());
                ASSERT_EQ(keys_int32[start_position].compare( iterators.first.getKey()), 0);
                iterators.first++;
                start_position++;
            }
            ASSERT_EQ(start_position, end_position);
        }


        TEST_F (SuRFInt32Test, IteratorTestsRangeLookupInclusiveTest) {
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            size_t start_position = 7234;
            size_t end_position = 7235;
            auto iterators = surf->lookupRange(keys_int32[start_position - 1], false, keys_int32[end_position], false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position]), 0);

            iterators = surf->lookupRange(keys_int32[start_position - 1], false, keys_int32[end_position], true);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position + 1]), 0);

            iterators = surf->lookupRange(keys_int32[start_position], true, keys_int32[end_position], true);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position + 1]), 0);

            iterators = surf->lookupRange(uint32ToString(2), true, uint32ToString(5), false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[0]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[1]), 0);
        }

        TEST_F (SuRFInt32Test, IteratorTestsRangeLookupRightBoundaryTest) {
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            size_t start_position = keys_int32.size() - 10;
            size_t end_position = keys_int32.size() - 1;
            auto iterators = surf->lookupRange(keys_int32[start_position - 1], false, keys_int32[end_position], false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position]), 0);

            iterators = surf->lookupRange(keys_int32[start_position - 1], false, keys_int32[end_position], true);

            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_FALSE(iterators.second.isValid());

            while (iterators.first != iterators.second) {
                ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
                ASSERT_TRUE(iterators.first.isValid());

                start_position++;
                iterators.first++;
            }

            ASSERT_EQ(start_position, keys_int32.size());
        }

        TEST_F (SuRFInt32Test, IteratorTestsRangeLookupLeftBoundaryTest) {
            SuRF *surf = new SuRF(keys_int32, values_uint64, kIncludeDense, 128);
            size_t start_position = 0;
            size_t end_position = 10;
            auto iterators = surf->lookupRange(uint32ToString(0), false, keys_int32[end_position], false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position]), 0);


            iterators = surf->lookupRange(keys_int32[start_position], true, keys_int32[end_position], false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position]), 0);

            iterators = surf->lookupRange(keys_int32[start_position], false, keys_int32[end_position], false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position + 1]), 0);
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position]), 0);


            iterators = surf->lookupRange(uint32ToString(0), false, uint32ToString(2), false);

            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_TRUE(iterators.second.isValid());
            ASSERT_FALSE(iterators.first != iterators.second);

            // left smaller than right
            iterators = surf->lookupRange(keys_int32[123], false, keys_int32[23], false);

            ASSERT_FALSE(iterators.first.isValid());
            ASSERT_FALSE(iterators.second.isValid());
            ASSERT_FALSE(iterators.first != iterators.second);


            return;
            ASSERT_EQ(iterators.second.getKey().compare(keys_int32[end_position]), 0);
            return;
            iterators = surf->lookupRange(keys_int32[start_position - 1], false, keys_int32[end_position], true);

            ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
            ASSERT_TRUE(iterators.first.isValid());
            ASSERT_FALSE(iterators.second.isValid());

            while (iterators.first != iterators.second) {
                ASSERT_EQ(iterators.first.getKey().compare(keys_int32[start_position]), 0);
                ASSERT_TRUE(iterators.first.isValid());

                start_position++;
                iterators.first++;
            }

            ASSERT_EQ(start_position, keys_int32.size());
        }


    } // namespace surftest


} // namespace surf

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}