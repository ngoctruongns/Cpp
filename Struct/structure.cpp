#ifdef __cplusplus
extern "C"
#endif

#include <stdio.h>

struct student {
    char* ptrName;
    int age;
    float gpa;
};

int main() {
    struct student s;

    printf("Enter student name: ");
    scanf("%s", );

    printf("Enter student age: ");
    scanf("%d", &s.age);

    printf("Enter student GPA: ");
    scanf("%f", &s.gpa);

    printf("Student's name: %s\n", s.name);
    printf("Student's age: %d\n", s.age);
    printf("Student's GPA: %.2f\n", s.gpa);

    return 0;
}
