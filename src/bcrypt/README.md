# libbcrypt
Contains a stripped down version of the bcrypt library implementation from 
https://github.com/trusch/libbcrypt

## How to use this
Here an example how to use this wrapper class (you can find it in the src/ subdirectory)

```cpp
#include "BCrypt.hpp"
#include <iostream>

int main(){
	BCrypt bcrypt;
	std::string password = "test";
	std::string hash = bcrypt.generateHash(password);
	std::cout<<bcrypt.validatePassword(password,hash)<<std::endl;
	std::cout<<bcrypt.validatePassword("test1",hash)<<std::endl;
	return 0;
}
```

