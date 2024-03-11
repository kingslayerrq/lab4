#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int* reform(int* arr, int len, int exclude){
    int* res = malloc((len-1) * sizeof(int));
    for (int i = 0; i < len; i++){
        if (arr[i] == exclude){
            for (int j = i; j < len-1; j++){
                res[j] = arr[j+1];
            }
            break;
        }
        else{
            res[i] = arr[i];
        }
    }
    return res;
}
void combination(int *arr, int **res, int *r, int start, int end, int index, int span)
{
    if (index == span)
    {
        for (int i = 0; i < span; i++)
        {
            // fprintf(stdout, "%d,", r[i]);
        }
        // fprintf(stdout, "\n");
        int k = 0;
        while (res[k] != 0)
        {
            k++;
        }
        // fprintf(stdout, "k is : %d\n", k);
        res[k] = malloc(sizeof(int) * span);
        for (int i = 0; i < span; i++)
        {
            res[k][i] = r[i];
        }
        return;
    }

    for (int i = start; i <= end && end - i + 1 >= span - index; i++)
    {
        r[index] = arr[i];
        combination(arr, res, r, i + 1, end, index + 1, span);
        
        // fprintf(stdout, "r[%d] is %d\n", index, arr[i]);

        // index = 0;
    }
}
void swap(int* a, int *b){
    int temp = *a;
    *a = *b;
    *b = temp;
}
void permutation(int *arr, int **res, int l, int r, int span)
{
    int i;
    if(l==r){
        int* r = malloc(sizeof(int) * span);
        // for (int j = 0; j< span; j++){
        //     r[j] = arr[j];
        // }
        int k = 0;
        while (res[k] != 0){
            k++;
        }
        res[k] = malloc(sizeof(int) * span);
        for (int j = 0; j< span; j++){
            res[k][j] = arr[j];
        }
        
        return;
    }
    else{
        for(i = l; i <= r; i++){
            swap((arr+l), (arr+i));
            permutation(arr, res, l+1, r, span);
            swap((arr+l), (arr+i));
        }
    }
}
int main()
{
    int *input = malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++)
    {
        input[i] = i + 1;
    }
    //input[6] = -1;
    int* new;
    new = reform(input, 5, 2);
    for (int i = 0; i < 4; i++){
        fprintf(stdout, "%d, ", new[i]);
    }
    fprintf(stdout, "\n");
    int **res = calloc(4, sizeof(int *));
    int *r = malloc(sizeof(int) * 3);
    combination(new, res, r, 0, 3, 0, 3);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            fprintf(stdout, "%d,", res[i][j]);
        }
        fprintf(stdout, "\n");
    }

    int **perm = calloc(24, sizeof(int*));
    for (int i = 0; i < 4; i++){
        permutation(res[i], perm, 0, 2, 3);
    }
    fprintf(stdout, "perm\n");
    for (int i = 0; i < 24; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            fprintf(stdout, "%d,", perm[i][j]);
        }
        fprintf(stdout, "\n");
    }
    free(r);
    free(res);
    free(perm);
}