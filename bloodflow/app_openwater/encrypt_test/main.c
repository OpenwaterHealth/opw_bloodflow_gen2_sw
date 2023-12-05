#include <string.h>

#include "../encrypt_data.h"

const unsigned char *test_data =
    "Four score and seven years ago our fathers brought forth on this continent,\n\
a new nation, conceived in Liberty, and dedicated to the proposition that all\n\
men are created equal.\n\n\
Now we are engaged in a great civil war, testing whether that nation, or any\n\
nation so conceived and so dedicated, can long endure. We are met on a great\n\
battle-field of that war. We have come to dedicate a portion of that field, as\n\
a final resting place for those who here gave their lives that that nation might\n\
live. It is altogether fitting and proper that we should do this.\n\n\
But, in a larger sense, we can not dedicate -- we can not consecrate -- we can\n\
not hallow -- this ground. The brave men, living and dead, who struggled here,\n\
have consecrated it, far above our poor power to add or detract. The world will\n\
little note, nor long remember what we say here, but it can never forget what\n\
they did here. It is for us the living, rather, to be dedicated here to the\n\
unfinished work which they who fought here have thus far so nobly advanced. It is\n\
rather for us to be here dedicated to the great task remaining before us -- that\n\
from these honored dead we take increased devotion to that cause for which they\n\
gave the last full measure of devotion -- that we here highly resolve that these\n\
dead shall not have died in vain -- that this nation, under God, shall have a new\n\
birth of freedom -- and that government of the people, by the people, for the\n\
people, shall not perish from the earth.\n\n\
Abraham Lincoln\n\
November 19, 1863";

int main()
{
    encryptData("/home/steve/data/patient_data", (void *)test_data, strlen(test_data));
}
