#include "hms/HospitalSystem.h"

int main()
{
    hms::HospitalSystem hospital;
    hospital.displayStartupRecords();
    hospital.run();

    return 0;
}
