/*-------------------------------------------------------------------------------
tucano is a chess playing engine developed by Alcides Schulz.
Copyright (C) 2011-present - Alcides Schulz

tucano is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

tucano is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You can find the GNU General Public License at http://www.gnu.org/licenses/
-------------------------------------------------------------------------------*/

#include "globals.h"

//-------------------------------------------------------------------------------------------------
//  NN training. Input = 2 colors * 6 types * 64 squares, Hidden = 512 neurons and 1 output
//-------------------------------------------------------------------------------------------------

#define TNN_INDEX    32
#define TNN_INPUT    (6 * 64 * 2)
#define TNN_HIDDEN   512

#define MAX_FILES    100

const float WDL_WEIGHT = 0.0f;
const float EVAL_WEIGHT = 1.0f;

const float GRAD_BETA1 = 0.9f;
const float GRAD_BETA2 = 0.999f;
//const float GRAD_EPSILON = 1e-8f;
const float GRAD_EPSILON = 0.001f;

const float SIGMOID_SCALE = 4.0f / 1024.0f;

float LEARN_RATE = 0.01f;
float DECAY_RATE = 0.50f;

// Settings when running on my windows and linux development environments.
#ifdef _MSC_VER
#define TRAIN_COUNT 10000000
#define VALID_COUNT 1000000
#define TNN_DATA_SOURCE "d:/temp/tnn/d%03d.tnn"
#define TNN_DATA_VALID  "d:/temp/tnn/valid.tnn"
#define TNN_EXPORT_NAME "d:/temp/tnn/tucanno%04.8f-%04d.%s"
#define TNN_THREAD_MAX 8
#define TNN_TEMP_SOURCE "d:/temp/tnn/temp/d%03d.tnn"
#define TNN_EVAL_MAX 6000
#define TNN_RESUME_FILE "d:/temp/tucanno.bin"
#else
#define TRAIN_COUNT 50000000
#define VALID_COUNT 10000000
#define TNN_DATA_SOURCE "/home/alcides/mynet/data/d%03d.tnn"
#define TNN_DATA_VALID  "/home/alcides/mynet/data/valid.tnn"
#define TNN_EXPORT_NAME "/home/alcides/mynet1/export/tucanno%04.8f-%04d.%s"
#define TNN_THREAD_MAX 10
#define TNN_TEMP_SOURCE "/home/alcides/mynet/data/temp/d%03d.tnn"
#define TNN_EVAL_MAX 6000
#define TNN_RESUME_FILE "/home/alcides/mynet1/tucanno.bin"
#endif

typedef struct s_grad {
    float  g;
    float  m;
    float  v;
}   TGRAD;

typedef struct s_gradient {
    TGRAD   grad_input_weight[TNN_INPUT][TNN_HIDDEN];
    TGRAD   grad_input_bias[TNN_HIDDEN];
    TGRAD   grad_hidden_weight[TNN_HIDDEN];
    TGRAD   grad_hidden_bias;
}   GRADIENT;

typedef struct s_nn {
    float  input_weight[TNN_INPUT][TNN_HIDDEN];
    float  input_bias[TNN_HIDDEN];
    float  hidden_weight[TNN_HIDDEN];
    float  hidden_bias;
}   TNN;

typedef struct s_record {
    S16     input[TNN_INDEX];
    S16     eval;
    S16     result;
}   TSAMPLE;

typedef struct s_batch_thread
{
    THREAD_ID   id;
    int         thread_number;
    TSAMPLE     *record;
    int         start_index;
    int         end_index;
    float       total_error;
    float       grad_input_weight[TNN_INPUT][TNN_HIDDEN];
    float       grad_input_bias[TNN_HIDDEN];
    float       grad_hidden_weight[TNN_HIDDEN];
    float       grad_hidden_bias;
}   BATCH_THREAD;

GRADIENT        gradient;
BATCH_THREAD    batch_thread[TNN_THREAD_MAX];
TNN             network;
int             data_file_list[MAX_FILES];
int             data_file_count = 0;

float sigmoid(float value)
{
    return 1.0f / (1.0f + expf(-value * SIGMOID_SCALE));
}

float relu_derivative(float x)
{
    return x > 0 ? 1.0f : 0;
}

float tnn_rand(float low, float high)
{
    return ((float)rand() * (high - low)) / (float)RAND_MAX + low;
}

//void tnn_rebuild_weight(TNN *nn)
//{
//    memset(nn, 0, sizeof(TNN));
//    for (int i = 0; i < TNN_INPUT; i++) {
//        for (int j = 0; j < TNN_HIDDEN; j++) {
//            nn->input_weight[i][j] = (float)TNN_INPUT_WEIGHT[i][j] / 16.0f;
//        }
//    }
//    for (int i = 0; i < TNN_HIDDEN; i++) {
//        nn->input_bias[i] = (float)TNN_INPUT_BIAS[i] / 16.0f;
//        nn->hidden_weight[i] = (float)TNN_HIDDEN_WEIGHT[i] / 16.0f;
//    }
//    nn->hidden_bias = (float)TNN_HIDDEN_BIAS / 16.0f;
//}

void tnn_init_weight(TNN *nn)
{
    memset(nn, 0, sizeof(TNN));
    float min = -1;
    float max = 1;

    for (int i = 0; i < TNN_INPUT; i++) {
        for (int j = 0; j < TNN_HIDDEN; j++) {
            nn->input_weight[i][j] = tnn_rand(min, max);
        }
    }
    for (int i = 0; i < TNN_HIDDEN; i++) {
        nn->input_bias[i] = 0;//tnn_rand(min, max);
        nn->hidden_weight[i] = tnn_rand(min, max);
    }
    nn->hidden_bias = 0;//tnn_rand(min, max);
}

void tnn_load_from_bin(TNN *nn, char *file_name)
{
    FILE *import_file = fopen(file_name, "rb");

    printf("importing .bin file: %s...\n", file_name);
    fread(nn, sizeof(TNN), 1, import_file);
    fclose(import_file);
}

void tnn_write_record(FILE *bin_file, char *fen, int eval) 
{
    TSAMPLE record;
    tnn_fen2index(fen, record.input);
    int side_to_move = (strstr(fen, " w") != NULL ? WHITE : (strstr(fen, " b") != NULL ? BLACK : -1));
    if (side_to_move == -1) {
        fprintf(stderr, "fen missing side to move, fen: %s\n", fen);
        return;
    }
    record.eval = (S16)(side_to_move == WHITE ? eval : -eval);
    fwrite(&record, sizeof(TSAMPLE), 1, bin_file);
}

void tnn_test_file(char *plain_file_name, char *bin_file_name)
{
    FILE *plain_file = fopen(plain_file_name, "r");
    if (!plain_file) {
        printf("error: cannot open file %s\n", plain_file_name);
        return;
    }
    FILE *bin_file = fopen(bin_file_name, "rb");
    if (!bin_file) {
        printf("error: cannot open file %s\n", bin_file_name);
        fclose(plain_file);
        return;
    }

    char line[1000];
    TSAMPLE record;
    TSAMPLE saved;
    int count = 0;

    while (fgets(line, 1000, plain_file) != NULL) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (!strncmp(line, "fen ", 4)) {
            char *fen = &line[4];
            tnn_fen2index(fen, record.input);
            count++;
            if (count % 100000 == 0) printf("%d %s                    \r", count, fen);
        }
        if (!strncmp(line, "score ", 4)) {
            record.eval = (S16)atoi(&line[6]);
            fread(&saved, sizeof(TSAMPLE), 1, bin_file);
            assert(memcmp(&record, &saved, sizeof(TSAMPLE)) == 0);
        }
    }

    fclose(plain_file);
    fclose(bin_file);

    printf("\ndone.\n");
}

void tnn_test(char *file_name) 
{
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        printf("error opening: %s\n", file_name);
        exit(-1);
    }
    TSAMPLE r;
    int c = 0;
    while (fread(&r, sizeof(TSAMPLE), 1, f)) {
        c++;
    }
    printf("%d records read from %s\n", c, file_name);
    fclose(f);
    getchar();
}

void tnn_export_bin(TNN *nn, float average_error, int epoch)
{
    char    file_name[1000];
    sprintf(file_name, TNN_EXPORT_NAME, average_error, epoch, "bin");
    FILE *export_file = fopen(file_name, "wb");
    printf("exporting .bin file: %s...\n", file_name);
    fwrite(nn, sizeof(TNN), 1, export_file);
    fclose(export_file);
}

void tnn_export_h(TNN *nn, float average_error, int epoch)
{
    char    file_name[1000];

    sprintf(file_name, TNN_EXPORT_NAME, average_error, epoch, "h");
    FILE *export_file = fopen(file_name, "w");

    printf("exporting .h file: %s...\n", file_name);

    // constants
    fprintf(export_file, "//------------------------------------------------------------------------------------\n");
    fprintf(export_file, "// Tucano Neural Network (epoch: %d)\n", epoch);
    fprintf(export_file, "//------------------------------------------------------------------------------------\n\n");
    fprintf(export_file, "#define TNN_INDEX_SIZE %d\n", TNN_INDEX);
    fprintf(export_file, "#define TNN_INPUT_SIZE %d\n", TNN_INPUT);
    fprintf(export_file, "#define TNN_HIDDEN_SIZE %d\n", TNN_HIDDEN);
    fprintf(export_file, "\n");
    
    // input weight
    fprintf(export_file, "static S16 TNN_INPUT_WEIGHT[TNN_INPUT_SIZE][TNN_HIDDEN_SIZE] = {\n");
    for (int i = 0; i < TNN_INPUT; i++) {
        fprintf(export_file, "    {");
        for (int j = 0; j < TNN_HIDDEN; j++) {
            if (j > 0) fprintf(export_file, ",");
            fprintf(export_file, "%d", (S16)(nn->input_weight[i][j] * 64.0));
        }
        fprintf(export_file, "}");
        if (i != TNN_INPUT - 1) fprintf(export_file, ",");
        fprintf(export_file, "\n");
    }
    fprintf(export_file, "};\n");
    // input bias
    fprintf(export_file, "static S16 TNN_INPUT_BIAS[TNN_HIDDEN_SIZE] = {");
    for (int i = 0; i < TNN_HIDDEN; i++) {
        if (i > 0) fprintf(export_file, ",");
        fprintf(export_file, "%d", (S16)(nn->input_bias[i] * 64.0));
    }
    fprintf(export_file, "};\n");
    // hidden weight
    fprintf(export_file, "static S16 TNN_HIDDEN_WEIGHT[TNN_HIDDEN_SIZE] = {");
    for (int i = 0; i < TNN_HIDDEN; i++) {
        if (i > 0) fprintf(export_file, ",");
        fprintf(export_file, "%d", (S16)(nn->hidden_weight[i] * 64.0));
    }
    fprintf(export_file, "};\n");
    // hidden bias
    fprintf(export_file, "static S16 TNN_HIDDEN_BIAS = %d;\n", (S16)(nn->hidden_bias * 64.0));

    fclose(export_file);
}

float tnn_forward_propagate(TNN *nn, S16 input_index[], float hidden_neuron[])
{
    for (int i = 0; i < TNN_HIDDEN; i++) {
        hidden_neuron[i] = nn->input_bias[i];
    }
    for (int i = 0; i < TNN_INDEX && input_index[i] != -1; i++) {
        for (int j = 0; j < TNN_HIDDEN; j++) {
            hidden_neuron[j] += nn->input_weight[input_index[i]][j];
        }
    }
    float output_value = nn->hidden_bias;
    for (int i = 0; i < TNN_HIDDEN; i++) {
        hidden_neuron[i] = MAX(0, hidden_neuron[i]);
        output_value += hidden_neuron[i] * nn->hidden_weight[i];
    }
    return output_value;
}

void tnn_update_gradient(float* value, TGRAD* grad) {
    if (!grad->g) return;

    grad->m = GRAD_BETA1 * grad->m + (1.0f - GRAD_BETA1) * grad->g;
    grad->v = GRAD_BETA2 * grad->v + (1.0f - GRAD_BETA2) * grad->g * grad->g;
    float delta = LEARN_RATE * grad->m / (sqrtf(grad->v) + GRAD_EPSILON);

    *value -= delta;

    grad->g = 0;
}

void tnn_apply_gradients(TNN *nn, GRADIENT *grad)
{
    for (int i = 0; i < TNN_INPUT; i++) {
        for (int j = 0; j < TNN_HIDDEN; j++) {
            tnn_update_gradient(&nn->input_weight[i][j], &grad->grad_input_weight[i][j]);
        }
    }
    for (int i = 0; i < TNN_HIDDEN; i++) {
        tnn_update_gradient(&nn->input_bias[i], &grad->grad_input_bias[i]);
        tnn_update_gradient(&nn->hidden_weight[i], &grad->grad_hidden_weight[i]);
    }
    tnn_update_gradient(&nn->hidden_bias, &grad->grad_hidden_bias);
}


//rnbqkb2/pppppppr/5n1p/4N3/8/3P4/PPPQPPPP/RNB1KB1R b - -;score=-4;[1/2]
void tnn_line2record(char *line, TSAMPLE *record)
{
    tnn_fen2index(line, record->input);
    record->eval = 0;
    record->result = 1;
    char *eval = strstr(line, "score=");
    if (eval == NULL) return;
    record->eval = (S16)(atoi(eval + 6));
    if (strstr(line, "[1-0]")) record->result = 2;
    if (strstr(line, "[1/2]")) record->result = 1;
    if (strstr(line, "[0-1]")) record->result = 0;
    //printf("eval: %d result: %d %s", record->eval, record->result, line);
    //getchar();
}

size_t tnn_load(int file_number, size_t max_records, TSAMPLE *record)
{
    char file_name[1000];
    size_t record_count = 0;
    char line[1024];

    sprintf(file_name, TNN_DATA_SOURCE, file_number);
    FILE *file = fopen(file_name, "r");
    if (file == NULL) return 0;

    int interval = (int)max_records / 10;
    while (fgets(line, 1024, file) != NULL && record_count < max_records) {
        tnn_line2record(line, &record[record_count]);
        if (ABS(record[record_count].eval) > TNN_EVAL_MAX) continue;
        record_count++;
        if (record_count % interval == 0) {
            printf("loading %s, %d records...\r", file_name, (int)record_count);
            fflush(stdout);
        }
    }
    printf("\n");

    fclose(file);

    return record_count;
}

float tnn_error(float estimate, TSAMPLE *record)
{
    float estimate_sigmoid = sigmoid(estimate);
    float target_sigmoid = sigmoid(record->eval);

    float cost = 0;
    cost += EVAL_WEIGHT * powf(estimate_sigmoid - target_sigmoid, 2.0f);
    cost += WDL_WEIGHT * powf(estimate_sigmoid - record->result / 2.0f, 2.0f);
    return cost;
}

float tnn_validate()
{
    char line[1024];
    int record_count = 0;
    double cost_total = 0;
    TSAMPLE record;
    float hidden_neuron[TNN_HIDDEN];

    FILE *file = fopen(TNN_DATA_VALID, "r");
    if (file == NULL) return 0;

    printf("validation file %s, %d positions, ", TNN_DATA_VALID, VALID_COUNT);

    while (fgets(line, 1024, file) != NULL && record_count < VALID_COUNT) {
        tnn_line2record(line, &record);
        float estimate = tnn_forward_propagate(&network, record.input, hidden_neuron);
        cost_total += tnn_error(estimate, &record);
        record_count++;
    }

    fclose(file);

    double average_error = cost_total / record_count;

    printf("average error: %.8f\n", average_error);

    return (float)average_error;
}

void *tnn_batch(void *data)
{
    BATCH_THREAD *batch = (BATCH_THREAD *)data;

    //printf("thread: %d start: %d end: %d\n", batch->thread_number, batch->start_index, batch->end_index);

    float hidden_neuron[TNN_HIDDEN];

    for (int rec_num = batch->start_index; rec_num < batch->end_index; rec_num++) {
        TSAMPLE *current = &batch->record[rec_num];
        
        float output = tnn_forward_propagate(&network, current->input, hidden_neuron);
        float target = (float)current->eval;

        batch->total_error += tnn_error(output, current);

        float output_sigmoid = sigmoid(output);
        float target_sigmoid = sigmoid(target);

        float sigmoid_prime = output_sigmoid * (1.0f - output_sigmoid) * SIGMOID_SCALE;
        float error_gradient = 0;
        error_gradient += WDL_WEIGHT * (output_sigmoid - current->result / 2.0f);
        error_gradient += EVAL_WEIGHT * (output_sigmoid - target_sigmoid);

        float output_loss = sigmoid_prime * error_gradient;

        float hidden_losses[TNN_HIDDEN];
        for (int i = 0; i < TNN_HIDDEN; i++) {
            hidden_losses[i] = output_loss * network.hidden_weight[i] * relu_derivative(hidden_neuron[i]);
        }

        batch->grad_hidden_bias += output_loss;
        for (int i = 0; i < TNN_HIDDEN; i++) {
            batch->grad_hidden_weight[i] += hidden_neuron[i] * output_loss;
        }

        for (int i = 0; i < TNN_HIDDEN; i++) {
            batch->grad_input_bias[i] += hidden_losses[i];
        }

        for (int i = 0; i < TNN_INDEX && current->input[i] != -1; i++) {
            for (int j = 0; j < TNN_HIDDEN; j++) {
                batch->grad_input_weight[current->input[i]][j] += hidden_losses[j];
            }
        }
    }

    return NULL;
}

void tnn_shuffle_samples(TSAMPLE record_list[], size_t record_count)
{
    TSAMPLE temp;
    for (size_t i = 0; i < record_count; i++) {
        size_t swap_index = (rand() * rand()) % record_count;
        temp = record_list[swap_index];
        record_list[swap_index] = record_list[i];
        record_list[i] = temp;
    }
}

void tnn_prepare_files()
{
    char file_name[1000];
    data_file_count = 0;
    printf("preparing file list from: %s...\n", TNN_DATA_SOURCE);
    for (int i = 0; i < MAX_FILES; i++) {
        sprintf(file_name, TNN_DATA_SOURCE, i);
        FILE *f = fopen(file_name, "r");
        if (!f) continue;
        fclose(f);
        data_file_list[data_file_count++] = i;
        printf("  -> file '%s' accepted.\n", file_name);
    }
}

void tnn_shuffle_files()
{
    for (int i = 0; i < data_file_count; i++) {
        int swap_index = rand() % data_file_count;
        int temp = data_file_list[i];
        data_file_list[i] = data_file_list[swap_index];
        data_file_list[swap_index] = temp;
    }
}

void tnn_train(int epoch_total, int batch_size)
{
    //tnn_load_from_bin(&network, TNN_RESUME_FILE);
    tnn_init_weight(&network);

    //tnn_rebuild_weight(&nn);

    TSAMPLE *record_list;
    record_list = malloc(sizeof(TSAMPLE) * TRAIN_COUNT);
    if (!record_list) {
        printf("error allocating memory for records\n");
        exit(-1);
    }

    memset(&gradient, 0, sizeof(GRADIENT));

    tnn_prepare_files();

    float lowest_error = 0.0;
    double total_batch_error = 0;
    double total_batch_records = 0;

    printf("\nlearn rate: %.8f decay rate: %.8f threads: %d data files: %d max eval: %d\n\n", 
        LEARN_RATE, DECAY_RATE, TNN_THREAD_MAX, data_file_count, TNN_EVAL_MAX);

    for (int epoch = 1; epoch <= epoch_total; epoch++) {
        size_t record_total = 0;

        unsigned start_time = util_get_time();

        tnn_shuffle_files();

        for (int data_file_index = 0; data_file_index < data_file_count; data_file_index++) {
            size_t record_count = TRAIN_COUNT;
            if (data_file_index > 0) printf("\n");
            record_count = tnn_load(data_file_list[data_file_index], TRAIN_COUNT, record_list);
            if (record_count == 0) continue;

            printf("shuffling data...\r"); fflush(stdout);
            tnn_shuffle_samples(record_list, record_count);

            record_total += record_count;
            int batch_count = (int)record_count / batch_size;
            printf("file %d/%d: %d records read, records total: %u %d batches, batch size: %d\n", 
                data_file_index + 1, data_file_count, (int)record_count, (int)record_total, (int)batch_count, (int)batch_size);
            int batch_interval = (int)batch_count / 10;

            for (int batch_num = 0; batch_num < batch_count; batch_num++) {

                int start_index = batch_num * batch_size;
                int records_per_thread = batch_size / TNN_THREAD_MAX;

                for (int t = 0; t < TNN_THREAD_MAX; t++) {
                    memset(&batch_thread[t], 0, sizeof(BATCH_THREAD));
                    batch_thread[t].thread_number = t;
                    batch_thread[t].record = record_list;
                    batch_thread[t].start_index = start_index + t * records_per_thread;
                    batch_thread[t].end_index = start_index + t * records_per_thread + records_per_thread;
                    THREAD_CREATE(batch_thread[t].id, tnn_batch, &batch_thread[t]);
                }

                for (int t = 0; t < TNN_THREAD_MAX; t++) {
                    
                    THREAD_WAIT(batch_thread[t].id);
                    
                    for (int i = 0; i < TNN_INPUT; i++) {
                        for (int j = 0; j < TNN_HIDDEN; j++) {
                            gradient.grad_input_weight[i][j].g += batch_thread[t].grad_input_weight[i][j];
                        }
                    }
                    for (int i = 0; i < TNN_HIDDEN; i++) {
                        gradient.grad_input_bias[i].g += batch_thread[t].grad_input_bias[i];
                        gradient.grad_hidden_weight[i].g += batch_thread[t].grad_hidden_weight[i];
                    }
                    gradient.grad_hidden_bias.g += batch_thread[t].grad_hidden_bias;

                    total_batch_error += batch_thread[t].total_error;
                    total_batch_records += records_per_thread;
                }

                if ((batch_num + 1) % batch_interval == 0) {
                    double train_error = total_batch_error / total_batch_records;
                    printf("epoch: %d file: %d batch %d/%d train error: %.8f learn rate: %.8f decay rate: %.8f\r", 
                        epoch, data_file_index + 1, batch_num + 1, (int)batch_count, train_error, LEARN_RATE, DECAY_RATE);
                    fflush(stdout);
                }
                tnn_apply_gradients(&network, &gradient);
            }
            printf("\n");
        }
        printf("\n");

        float current_error = tnn_validate();
        tnn_export_h(&network, current_error, epoch);
        if (epoch % 10 == 0) tnn_export_bin(&network, current_error, epoch);
        
        printf("current error: %.8f lowest error: %.8f learn rate: %.8f decay rate: %.8f\n", current_error, lowest_error, LEARN_RATE, DECAY_RATE);
        if (epoch == 1) {
            lowest_error = current_error;
        }
        if (current_error > lowest_error) {
            LEARN_RATE *= DECAY_RATE;
            printf(" ===> changed learn rate: %.8f\n", LEARN_RATE);
        }
        if (current_error < lowest_error) {
            lowest_error = current_error;
        }

        unsigned elapsed = util_get_time() - start_time;
        printf("elapsed time: %d seconds\n", (int)(elapsed / 1000));
        printf("------------------------------------------------------------------------------------------------------------\n\n");
    }
}

void tnn_prepare_data(int lines_per_file)
{
    char source_name[1000];
    FILE *source_file = NULL;
    char target_name[1000];
    FILE *target_file = NULL;
    int target_file_count = 0;
    int target_line_count = lines_per_file + 1;
    char line[1000];

    printf("preparing data: %s -> %s (lines per file: %d)\n", TNN_TEMP_SOURCE, TNN_DATA_SOURCE, lines_per_file);
    for (int i = 0; i < MAX_FILES; i++) {
        sprintf(source_name, TNN_TEMP_SOURCE, i);
        source_file = fopen(source_name, "r");
        if (!source_file) continue;
        printf("  reading file '%s'\n", source_name);
        while (fgets(line, 1000, source_file) != NULL) {
            if (target_line_count >= lines_per_file) {
                if (target_file) fclose(target_file);
                target_file_count++;
                sprintf(target_name, TNN_DATA_SOURCE, target_file_count);
                target_file = fopen(target_name, "w");
                if (!target_file) {
                    printf("error creating file: %s\n", target_name);
                    exit(-1);
                }
                target_line_count = 0;
                printf("    saving lines to '%s'\n", target_name);
            }
            fprintf(target_file, "%s", line);
            target_line_count++;
        }
        fclose(source_file);
    }
    if (target_file) fclose(target_file);
}

void tnn_main()
{
    //tnn_prepare_data(TRAIN_COUNT);
    tnn_train(1000, 20000);
}

// EOF
