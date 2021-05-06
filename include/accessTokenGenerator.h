#ifndef ACCESS_TOKEN_GENERATOR_H
#define ACCESS_TOKEN_GENERATOR_H

#include <string>

class AccessTokenGenerator
{
    public:
    const char* getToken(const char* key, const char* token);

    private:

    std::string lastKey;
    std::string lastToken;
    std::string accessToken;
};

#endif

