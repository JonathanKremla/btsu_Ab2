/**
*@file forkFFFT.C
*@author Jonathan Kremla
*@date last edit 11-12-2021
*@brief Program to calculate Cool-Turkey fast fourier transform
*@details This Program implents the Cool Turkey fast fourier Transform, taking an
*input of size 2^n of Complex Double Values and returning the fourier Transform
*of the given Signal. The Program forks itself twice in each recursion level and
*splits the input into two even sized outputs given to each child, until we
*eventually get an input of size 1, then on the return path of the recursion it
*calculates the Fourier transform.
**/
#include <stdio.h>
#include "forkFFT.h"

//Program Name
static char *programName = "<unset>";

/**
 * @brief implements addition for the Struct ComplexNumber representing
 * Complex Numbers.
 * @param a pointer to a ComplexNumber struct representing the first addant.
 * @param b pointer to a ComplexNumber struct representing the second addant.
 * @param c pointer to a ComplexNumber struct, in which the result of the sum 
 * is saved.
 */
static void add(compNum *a, compNum *b, compNum *c){
    c -> imaginary = a->imaginary + b->imaginary;
    c -> real = a->real + b->real;
}

/**
 * @brief implements multiplication for the Struct ComplexNumber representing
 * Complex Numbers.
 * @param a pointer to a ComplexNumber struct representing the first factor.
 * @param b pointer to a ComplexNumber struct representing the second factor.
 * @param c pointer to a ComplexNumber struct, in which the result of 
 * the multiplication is saved.
 */
static void multiply (compNum *a, compNum *b, compNum *c){
    c -> real = (a->real * b->real) - (a->imaginary * b->imaginary);
    c -> imaginary = (a->real * b->imaginary) + (a->imaginary * b->real);
}

/**
 * @brief implements subtraction for the Struct ComplexNumber representing
 * Complex Numbers.
 * @param a pointer to a ComplexNumber struct representing the minuend.
 * @param b pointer to a ComplexNumber struct representing the subtrahend.
 * @param c pointer to a ComplexNumber struct, in which the result of 
 * the subtraction is saved.
 */
static void subtract (compNum *a, compNum *b, compNum *c){
    c -> real = a->real - b->real;
    c -> imaginary = a->imaginary - b->imaginary;
}

/**
 * @brief converting a String to a Complex Number (struct Complex Number)
 * 
 * @param c pointer to which the result of the conversion is saved in.
 * @param input char array(string) to convert
 */
static void convert(compNum *c, char *input){
    char *tmp;
    c -> real = strtof(input, &tmp);

    if(*tmp == ' '){
        c->imaginary = strtof(tmp,&tmp);
    }
    else{
        c -> imaginary = 0;
    }
}
/**
 * @brief forks the programm resulting in a child and a parent Process
 * @details the function creates a chikd of forkFFT by calling fork() and exec(),
 * opening pipes to the parent process and saving the pipe fd´s in a dependencies struct, 
 * together with the childs pid.
 * @param dep pointer to dependencies struct in which the pipe fd´s and the childs pid will be saved.
 */

static void createChild(dependencies * dep){
    //pipe 1 parent -> child
    //pipe 2 child -> parent
    int pipe1[2],pipe2[2];
    if((pipe(pipe1)) == -1 || (pipe(pipe2)) == -1){
        fprintf(stderr, "failed at creating pipes in %s",programName);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        exit(EXIT_FAILURE);
    }
    dep -> pid = fork();
    switch (dep -> pid){
        case 0: //case child
            close(pipe1[1]);
            close(pipe2[0]);

            if((dup2(pipe1[0],STDIN_FILENO)) == -1){
                fprintf(stderr,"failed at dup %s",programName);
                close(pipe1[0]);
                close(pipe2[1]);
                exit(EXIT_FAILURE);
            }
            close(pipe1[0]);
            if((dup2(pipe2[1], STDOUT_FILENO)) == -1){
                fprintf(stderr,"failed at dup in %s",programName);
                close(pipe2[1]);
                exit(EXIT_FAILURE);
            }
            close(pipe2[1]);

            if(execlp(programName,programName,NULL) == -1){
                fprintf(stderr,"failed at excelp in %s",programName);
                exit(EXIT_FAILURE);
            }
            assert(0);
        case -1: //case error
            fprintf(stderr,"fork failed in %s",programName);
            exit(EXIT_FAILURE);
            break;
        default:
            close(pipe1[0]);
            close(pipe2[1]);
            dep -> write = pipe1[1]; //saving fd to dep.write
            dep -> read = pipe2[0]; //saving fd to dep.read

    }
}
/**
 * @brief calculates the Cool-Turkey FFT and writes it to stdout.
 * @param inputEven array containing the output from the Child to which we wrote even
 * indices.
 * @param inputOdd array containing the output from the Child to which we wrote odd
 * indices
 * @param size int which describes the size of inputtOdd/inputEven (same size).
 */

static void butterfly(char **inputEven, char **inputOdd, int size){
    compNum result[size * 2];
    
    for(int i = 0; i < size; i++){
        compNum even,odd,res,temp,holder;
        temp.real = cos(-2*PI/(size*2) * i);
        temp.imaginary = sin(-2*PI/(size*2) * i);
        convert(&even,inputEven[i]);
        convert(&odd,inputOdd[i]);
        multiply(&temp,&odd,&holder);
        add(&even, &holder, &res);
        result[i] = res;

        subtract(&even, &holder, &res);
        result[i + size] = res;
    }
    
    for(int i = 0; i < size * 2; i++){
        fprintf(stdout, "%f %f*i\n",result[i].real, result[i].imaginary);
    }
}

/**
 * @brief reads lines from stdin, calculates the FFT and calls butterfly.
 * @details The Program starts here, the main function provides the main functionality
 * by calling createChild() and butterfly().
 * @param argc argument count
 * @param argv argument vector
 * @return EXIT_SUCCES on succesful termination
 *         EXIT_FAILURE on failed termination
 **/
int main(int argc, char *argv[])
{
    argumentParsing(argc, argv);
    
    programName = argv[0];

    char *line1 = NULL;
    char *line2 = NULL;
    size_t n1 = 0;
    size_t n2 = 0;
    ssize_t length1 = 0;
    ssize_t length2 = 0;

    
    length1 = getline(&line1, &n1, stdin);
    if (length1 == -1) //No line was read, something went wrong in parent process.
    { 
        fprintf(stderr, "Too few lines in %s",programName);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }


    length2 = getline(&line2, &n2, stdin);
    if (length2 == -1) //only one line was read
    { 
        fprintf(stdout, "%s", line1);
        free(line1);
        free(line2);
        exit(EXIT_SUCCESS);
    }


    dependencies e_dep,o_dep;
    createChild(&e_dep);
    createChild(&o_dep);
    

    FILE * fEwr = fdopen(e_dep.write,"w");
    FILE * fOwr = fdopen(o_dep.write,"w");

    
    fprintf(fEwr,"%s",line1);
    fprintf(fOwr,"%s",line2);

    int count = 1; // We want to know how many floats we are writing to our children.
    while(1){
        if((getline(&line1,&n1,stdin)) == EOF){
            break;
        }
        fprintf(fEwr,"%s",line1);

        if((getline(&line2, &n2,stdin)) == EOF){
            free(line1);
            free(line2);
            fclose(fOwr);
            fclose(fEwr);
            usage(programName);
        }
        fprintf(fOwr,"%s",line2);
        count ++;
    }

    fflush(fEwr);
    fflush(fOwr);

    fclose(fEwr);
    fclose(fOwr);
    
    int status;
    waitpid(e_dep.pid,&status,0);
    waitpid(o_dep.pid,&status,0);


    FILE *fErd = fdopen(e_dep.read,"r");
    FILE *fOrd = fdopen(o_dep.read,"r");



    char *readCharsEven[count]; // we are reading the same number of chars we put into our children, therefore we can use count from before.
    char *readCharsOdd[count];
    
    int index = 0;
    while(getline(&line1, &n1, fErd) != -1){
        readCharsEven[index] = malloc(strlen(line1));
        strncpy(readCharsEven[index], line1, strlen(line1));
        index++;
    }
    
    index = 0;
    while(getline(&line2, &n2, fOrd) != -1){
        readCharsOdd[index] = malloc(strlen(line2));
        strncpy(readCharsOdd[index], line2, strlen(line2));
        index++;
    }
    


    free(line1);
    free(line2);
    fflush(fErd);
    fflush(fOrd);
    fflush(stdout);
    fflush(stdout);
    fclose(fErd);
    fclose(fOrd);

    butterfly(readCharsEven,readCharsOdd,count);


    for(int i = 0; i < count; i++){
        free(readCharsEven[i]);
        free(readCharsOdd[i]);
    }

}