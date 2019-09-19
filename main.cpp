#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>
#include <iostream>

#include <thread>
#include <map>
#include <setjmp.h>
#include <string>
#include <stack>


#define EXCEPTIN_MESSAGE_LENGTH     512

typedef std::string Exception;


class ExceptionFrame {
public:
    ExceptionFrame():line(0){}
    //jmp上下文
    jmp_buf env;
    //异常显示的行数
    int line;
    //函数名
    std::string func;
    //文件名
    std::string file;
    //异常名字
    Exception exception;
    std::string message;
};

//当前线程的异常框架栈
thread_local std::stack<ExceptionFrame*> g_local_map;

//压栈
#define ThreadDataSet(frame)    g_local_map.push(frame) 
//获取栈顶数据
#define ThreadDataGet           (g_local_map.empty()? nullptr:g_local_map.top()) 
//弹出栈
#define ExceptionPopStack       if(g_local_map.size()){g_local_map.pop();}

//往上一层抛异常
#define ReThrow                                                                                 \
{                                                                                               \
ExceptionFrame* _frame = ThreadDataGet;                                                      \
if (_frame) { ExceptionThrow(frame.exception, frame.func, frame.file, frame.line, NULL);};   \
}                                                                                               \
//抛异常
#define Throw(e, cause, ...)       ExceptionThrow((e), __func__, __FILE__, __LINE__, cause, ##__VA_ARGS__, NULL)

//tryCatch状态
enum {
    ExceptionEntered = 0,
    ExceptionThrown,
    ExceptionHandled,
    ExceptionFinalized
};


#define Try do {                                                              \
            ExceptionFrame frame;                                             \
            frame.message[0] = 0;                                             \
            ThreadDataSet(&frame);                                         \
            volatile int Exception_flag = setjmp(frame.env);                  \
            if (Exception_flag == ExceptionEntered) {    


#define Catch(e)                                                              \
                if (Exception_flag == ExceptionEntered) ExceptionPopStack; \
            } else if (frame.exception == (e)) {                              \
                Exception_flag = ExceptionHandled;                            \


#define Finally                                                               \
                if (Exception_flag == ExceptionEntered) ExceptionPopStack; \
            } {                                                               \
                if (Exception_flag == ExceptionEntered)                       \
                    Exception_flag = ExceptionFinalized;                      \

#define EndTry                                                                \
                if (Exception_flag == ExceptionEntered) ExceptionPopStack; \
            } if (Exception_flag == ExceptionThrown) ReThrow;                 \
            } while (0)                                                       \



void ExceptionThrow(Exception excep, std::string func, std::string file, int line, const std::string& cause, ...) {

    ExceptionFrame *frame = ThreadDataGet;

    if (frame) {
        frame->exception = excep;
        frame->func = func;
        frame->file = file;
        frame->line = line;
        frame->message = cause;
        ExceptionPopStack;
        longjmp(frame->env, ExceptionThrown);
    }
    else{
        printf("%s: \n raised in %s at %s:%d\n", excep.c_str(), !func.empty() ? func.c_str() : "?", !file.empty() ? file.c_str() : "?", line);
    }
}


/* ** **** ******** **************** debug **************** ******** **** ** */

Exception A = "AException" ;
Exception B = "BException" ;
Exception C = "CException" ;
Exception D = "DException" ;

void threadFunc() {

    std::thread::id selfid = std::this_thread::get_id();
    Try{
        Throw(A, "A");
    } Catch(A) {
        printf("catch A : %d\n", selfid);
    } EndTry;

    Try{
        Throw(B, "B");
    } Catch(B) {
        printf("catch B : %d\n", selfid);
    } EndTry;

    Try{
        Throw(C, "C");
    } Catch(C) {
        printf("catch C : %d\n", selfid);
    } EndTry;

    Try{
        Throw(D, "D");
    } Catch(D) {
        printf("catch D : %d\n", selfid);
    } EndTry;

    Try{

        Throw(A, "A Again");
        Throw(B, "B Again");
        Throw(C, "C Again");
        Throw(D, "D Again");

    } Catch(A) {
        printf("catch A again : %d\n", selfid);
    } Catch(B) {
        printf("catch B again : %d\n", selfid);
    } Catch(C) {
        printf("catch C again : %d\n", selfid);
    } Catch(D) {
        printf("catch B again : %ld\n", selfid);
    } EndTry;
}


#define THREADS        50

int main(void) {

    Throw(D, "");

    Throw(C, "null C");

    printf("\n\n=> Test1: Try-Catch\n");

    Try{
        Try{
            Throw(B, "recall B");
        } Catch(B) {
            printf("recall B \n");
        } EndTry;
        Throw(B, "");
    } Catch(A) {
        printf("recall A \n");
    } EndTry;

    printf("=> Test1: Ok\n\n");

#if 0
    printf("=> Test2: Test Thread-safeness\n");
    for (int i = 0; i < THREADS; i++) {
        std::thread(threadFunc).join();
    }
    printf("=> Test2: Ok\n\n");
#endif
    for(;;);
    return 0;
}