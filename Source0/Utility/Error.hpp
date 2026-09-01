#define VKHPP_CHECK(RESULT_)                                                            \
    do {                                                                                \
        if (RESULT_.result != vk::Result::eSuccess) {                                   \
            printf("Vulkan Error : file : %s, line : %d, func : %s\n, %s\n", __FILE__, __LINE__, __func__, vk::to_string(RESULT_.result).c_str());    \
            exit(-1);                                                               \
        }                                                                           \
    } while (0)

#define ASSERT_WITH_MSG(RESULT_, MSG_)                                      \
    do {                                                                    \
        if (RESULT_ == false) {                                             \
            printf("Error : file : %s, line : %d, func : %s\n, %s\n",  __FILE__, __LINE__, __func__, std::string(MSG_).c_str());              \
            exit(-1);                                                       \
        }                                                                   \
    } while (0)

#define ASSERT_TRUE_WITH_MSG(RESULT_, MSG_)                                   \
    do {                                                                    \
        if (RESULT_ == true) {                                              \
            printf("Error : file : %s, line : %d, func : %s\n, %s\n", __FILE__, __LINE__, __func__, std::string(MSG_).c_str());              \
            exit(-1);                                                       \
        }                                                                   \
    } while (0)