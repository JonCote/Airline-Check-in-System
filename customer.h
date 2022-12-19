#ifndef _CUSTOMER_H_
#define _CUSTOMER_H_

typedef struct Customer
{
    int id;
    int class_type;
    int service_time;
    int arrival_time;
    double queued_time;
    double serv_start_time;

} Customer;

#endif