#include <stdio.h>
#include <vector>
#include <iostream>
#include "h1.h"
#include "folder/goo.h"
#include "folder/zoo.h"
#include "folder/folder2/boo.h"


int main() {
    std::cout << foo() << goo() << boo() << std::endl;
}