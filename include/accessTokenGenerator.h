#ifndef ACCESS_TOKEN_GENERATOR_H
#define ACCESS_TOKEN_GENERATOR_H

#include <string>

class AccessTokenGenerator
{
    public:
    void setToken(const char* token);
    const char* getAccessToken(const char* key);

    private:

    std::string lastKey;
    std::string lastToken;
    std::string accessToken;
};

#endif

