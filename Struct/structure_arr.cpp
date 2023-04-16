#include <stdio.h>

struct student {
    char name[50];
    int age;
};

int main() {
    int i;
    struct student s[3];

    for (i = 0; i < 3; i++) {
        printf("Enter student %d name: ", i+1);
        scanf("%s", s[i].name);

        printf("Enter student %d age: ", i+1);
        scanf("%d", &s[i].age);

    }

    for (i = 0; i < 3; i++) {
        printf("Student %d's name: %s\n", i+1, s[i].name);
        printf("Student %d's age: %d\n", i+1, s[i].age);
    }

    return 0;
}
