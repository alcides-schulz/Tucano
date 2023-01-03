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

//#define TNN_USE_PROC_FILE

//-------------------------------------------------------------------------------------------------
//  NN training
//-------------------------------------------------------------------------------------------------

#define TNN_INDEX    32
#define TNN_INPUT    (6 * 64 * 2)
#define TNN_HIDDEN   512

#define MAX_FILES    100

const double WDL_WEIGHT = 0.0f;
const double EVAL_WEIGHT = 1.0f;

const double GRAD_BETA1 = 0.9f;
const double GRAD_BETA2 = 0.999f;
const double GRAD_EPSILON = 1e-8f;

const double SIGMOID_SCALE = 4.0f / 1024.0f;

double LEARN_RATE = 0.01f;
double DECAY_RATE = 1.00f;
double LEARN_MIN = 0.00000001f;

// Settings when running on my windows and linux development environments.
#ifdef _MSC_VER
#define TNN_POS_PER_FILE 50000
#define VALID_COUNT 100000
#ifdef TNN_USE_PROC_FILE
#define TNN_DATA_SOURCE "d:/temp/tnn/d%03d.proc"
#define TNN_DATA_VALID  "d:/temp/tnn/valid.proc"
#else
#define TNN_DATA_SOURCE "d:/temp/tnn/d%03d.tnn"
#define TNN_DATA_VALID  "d:/temp/tnn/valid.tnn"
#endif
#define TNN_EXPORT_NAME "d:/temp/tnn/tucanno%04.8f-%04d.%s"
#define TNN_THREAD_MAX 5
#define TNN_TEMP_SOURCE "d:/temp/tnn/temp/d%03d.tnn"
#define TNN_EVAL_MAX 6000
#define TNN_RESUME_FILE "d:/temp/tucanno.bin"
#else
#define TNN_POS_PER_FILE 50000000
#define VALID_COUNT 100000000
#ifdef TNN_USE_PROC_FILE
#define TNN_DATA_SOURCE "/home/alcides/mynet/data/d%03d.proc"
#define TNN_DATA_VALID  "/home/alcides/mynet/data/valid.proc"
#else
#define TNN_DATA_SOURCE "/home/alcides/mynet/data/d%03d.tnn"
#define TNN_DATA_VALID  "/home/alcides/mynet/data/valid.tnn"
#endif
#define TNN_EXPORT_NAME "/home/alcides/mynet1/export/tucanno%04.8f-%04d.%s"
#define TNN_THREAD_MAX 8
#define TNN_TEMP_SOURCE "/home/alcides/mynet/data/temp/d%03d.tnn"
#define TNN_EVAL_MAX 6000
#define TNN_RESUME_FILE "/home/alcides/mynet1/tucanno.bin"
#endif

typedef struct s_grad {
    double  g;
    double  m;
    double  v;
}   TGRAD;

typedef struct s_gradient {
    TGRAD   grad_input_weight[TNN_INPUT][TNN_HIDDEN];
    TGRAD   grad_input_bias[TNN_HIDDEN];
    TGRAD   grad_hidden_weight[TNN_HIDDEN];
    TGRAD   grad_hidden_bias;
}   GRADIENT;

typedef struct s_nn {
    double  input_weight[TNN_INPUT][TNN_HIDDEN];
    double  input_bias[TNN_HIDDEN];
    double  hidden_weight[TNN_HIDDEN];
    double  hidden_bias;
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
    double      total_error;
    double      grad_input_weight[TNN_INPUT][TNN_HIDDEN];
    double      grad_input_bias[TNN_HIDDEN];
    double      grad_hidden_weight[TNN_HIDDEN];
    double      grad_hidden_bias;
}   BATCH_THREAD;

GRADIENT        gradient;
BATCH_THREAD    batch_thread[TNN_THREAD_MAX];
TNN             network;
int             data_file_list[MAX_FILES];
int             data_file_count = 0;

double sigmoid(double value)
{
    return 1.0f / (1.0f + exp(-value * SIGMOID_SCALE));
}

double relu_derivative(double x)
{
    return x > 0 ? 1.0f : 0;
}

double tnn_rand(double low, double high)
{
    return ((double)rand() * (high - low)) / (double)RAND_MAX + low;
}

void tnn_init_weight(TNN *nn)
{
    memset(nn, 0, sizeof(TNN));
    double min = -1;
    double max = 1;

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

    if (import_file == NULL) {
        printf("cannot open file: '%s'\n", file_name);
        return;
    }

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

void tnn_export_bin(TNN *nn, double average_error, int epoch)
{
    char    file_name[1000];
    sprintf(file_name, TNN_EXPORT_NAME, average_error, epoch, "bin");
    FILE *export_file = fopen(file_name, "wb");
    printf("exporting .bin file: %s...\n", file_name);
    fwrite(nn, sizeof(TNN), 1, export_file);
    fclose(export_file);
}

S16 tnn_get_export_value(double value)
{
    S32 export_value = (S16)(value * NN_QUANTIZATION);
    if (export_value > INT16_MAX) {
        printf("value %f exceeds INT16_MAX: %d\n", value, export_value);
    }
    return (S16)export_value;
}

void tnn_export_h(TNN *nn, double average_error, int epoch)
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
            fprintf(export_file, "%d", tnn_get_export_value(nn->input_weight[i][j]));
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
        fprintf(export_file, "%d", tnn_get_export_value(nn->input_bias[i]));
    }
    fprintf(export_file, "};\n");
    // hidden weight
    fprintf(export_file, "static S16 TNN_HIDDEN_WEIGHT[TNN_HIDDEN_SIZE] = {");
    for (int i = 0; i < TNN_HIDDEN; i++) {
        if (i > 0) fprintf(export_file, ",");
        fprintf(export_file, "%d", tnn_get_export_value(nn->hidden_weight[i]));
    }
    fprintf(export_file, "};\n");
    // hidden bias
    fprintf(export_file, "static S16 TNN_HIDDEN_BIAS = %d;\n", tnn_get_export_value(nn->hidden_bias));

    fclose(export_file);
}

double tnn_forward_propagate(TNN *nn, S16 input_index[], double hidden_neuron[])
{
    for (int i = 0; i < TNN_HIDDEN; i++) {
        hidden_neuron[i] = nn->input_bias[i];
    }
    for (int i = 0; i < TNN_INDEX && input_index[i] != -1; i++) {
        for (int j = 0; j < TNN_HIDDEN; j++) {
            hidden_neuron[j] += nn->input_weight[input_index[i]][j];
        }
    }
    double output_value = nn->hidden_bias;
    for (int i = 0; i < TNN_HIDDEN; i++) {
        hidden_neuron[i] = MAX(0, hidden_neuron[i]);
        output_value += hidden_neuron[i] * nn->hidden_weight[i];
    }
    return output_value;
}

void tnn_update_gradient(double* value, TGRAD* grad) {
    if (!grad->g) return;

    grad->m = GRAD_BETA1 * grad->m + (1.0f - GRAD_BETA1) * grad->g;
    grad->v = GRAD_BETA2 * grad->v + (1.0f - GRAD_BETA2) * grad->g * grad->g;
    double delta = LEARN_RATE * grad->m / (sqrt(grad->v) + GRAD_EPSILON);

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

#ifdef TNN_USE_PROC_FILE

#define TNN_READ_COUNT 10000

size_t tnn_load_proc_file(int file_number, size_t max_records, TSAMPLE *record)
{
    char file_name[1000];
    size_t record_count = 0;

    sprintf(file_name, TNN_DATA_SOURCE, file_number);
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) return 0;

    int interval = (int)max_records / 10;
    size_t read_count = MIN(TNN_READ_COUNT, max_records);
    while (record_count + read_count <= max_records) {
        read_count = fread(&record[record_count], sizeof(TSAMPLE), read_count, file);
        if (read_count == 0) break;
        record_count += read_count;
        read_count = MIN(TNN_READ_COUNT, max_records - record_count);
        if (record_count % interval == 0) {
            printf("loading %s, %d records...\r", file_name, (int)record_count);
            fflush(stdout);
        }
    }
    printf("\n");

    fclose(file);

    return record_count;
}
#else
size_t tnn_load_tnn_file(int file_number, size_t max_records, TSAMPLE *record)
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
        if (is_mate_score(record[record_count].eval)) record[record_count].eval /= 5;
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
#endif

size_t tnn_load(int file_number, size_t max_records, TSAMPLE *record)
{
#ifdef TNN_USE_PROC_FILE
    return tnn_load_proc_file(file_number, max_records, record);
#else
    return tnn_load_tnn_file(file_number, max_records, record);
#endif
}

double tnn_error(double estimate, TSAMPLE *record)
{
    double estimate_sigmoid = sigmoid(estimate);
    double target_sigmoid = sigmoid(record->eval);

    double cost = 0;
    cost += EVAL_WEIGHT * pow(estimate_sigmoid - target_sigmoid, 2.0f);
    cost += WDL_WEIGHT * pow(estimate_sigmoid - record->result / 2.0f, 2.0f);
    return cost;
}

#ifdef TNN_USE_PROC_FILE
double tnn_validate()
{
    int record_count = 0;
    double cost_total = 0;
    TSAMPLE record;
    double hidden_neuron[TNN_HIDDEN];

    FILE *file = fopen(TNN_DATA_VALID, "rb");
    if (file == NULL) return 0;

    printf("validation file %s, %d positions, ", TNN_DATA_VALID, VALID_COUNT);

    while (fread(&record, sizeof(TSAMPLE), 1, file) == 1 && record_count < VALID_COUNT) {
        double estimate = tnn_forward_propagate(&network, record.input, hidden_neuron);
        cost_total += tnn_error(estimate, &record);
        record_count++;
    }
    fclose(file);

    record_count = MAX(1, record_count);
    double average_error = cost_total / record_count;

    printf("average error: %.8f\n", average_error);

    return (double)average_error;
}
#else
double tnn_validate()
{
    char line[1024];
    int record_count = 0;
    double cost_total = 0;
    TSAMPLE record;
    double hidden_neuron[TNN_HIDDEN];

    FILE *file = fopen(TNN_DATA_VALID, "r");
    if (file == NULL) return 0;

    printf("validation file %s, %d positions, ", TNN_DATA_VALID, VALID_COUNT);

    while (fgets(line, 1024, file) != NULL && record_count < VALID_COUNT) {
        tnn_line2record(line, &record);
        double estimate = tnn_forward_propagate(&network, record.input, hidden_neuron);
        cost_total += tnn_error(estimate, &record);
        record_count++;
    }

    fclose(file);

    record_count = MAX(1, record_count);
    double average_error = cost_total / record_count;

    printf("average error: %.8f\n", average_error);

    return (double)average_error;
}
#endif

void *tnn_batch(void *data)
{
    BATCH_THREAD *batch = (BATCH_THREAD *)data;

    //printf("thread: %d start: %d end: %d\n", batch->thread_number, batch->start_index, batch->end_index);

    double hidden_neuron[TNN_HIDDEN];

    for (int rec_num = batch->start_index; rec_num < batch->end_index; rec_num++) {
        TSAMPLE *current = &batch->record[rec_num];
        
        double output = tnn_forward_propagate(&network, current->input, hidden_neuron);
        double target = (double)current->eval;

        batch->total_error += tnn_error(output, current);

        double output_sigmoid = sigmoid(output);
        double target_sigmoid = sigmoid(target);

        double sigmoid_prime = output_sigmoid * (1.0f - output_sigmoid) * SIGMOID_SCALE;
        double error_gradient = 0;
        error_gradient += WDL_WEIGHT * (output_sigmoid - current->result / 2.0f);
        error_gradient += EVAL_WEIGHT * (output_sigmoid - target_sigmoid);

        double output_loss = sigmoid_prime * error_gradient;

        double hidden_losses[TNN_HIDDEN];
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

void tnn_create_data_file_list()
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
    record_list = malloc(sizeof(TSAMPLE) * TNN_POS_PER_FILE);
    if (!record_list) {
        printf("error allocating memory for records\n");
        exit(-1);
    }

    memset(&gradient, 0, sizeof(GRADIENT));

    tnn_create_data_file_list();

    double lowest_error = 0.0;

    printf("\nlearn rate: %.8f decay rate: %.8f threads: %d data files: %d max eval: %d\n\n", 
        LEARN_RATE, DECAY_RATE, TNN_THREAD_MAX, data_file_count, TNN_EVAL_MAX);

    for (int epoch = 1; epoch <= epoch_total; epoch++) {
        size_t record_total = 0;
        double total_train_error = 0;
        double total_train_records = 0;

        unsigned start_time = util_get_time();

        tnn_shuffle_files();

        for (int data_file_index = 0; data_file_index < data_file_count; data_file_index++) {
            size_t record_count = TNN_POS_PER_FILE;
            double total_data_file_error = 0;
            double total_data_file_records = 0;

            if (data_file_index > 0) printf("\n");
            record_count = tnn_load(data_file_list[data_file_index], TNN_POS_PER_FILE, record_list);
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

                    total_train_error += batch_thread[t].total_error;
                    total_train_records += records_per_thread;

                    total_data_file_error += batch_thread[t].total_error;
                    total_data_file_records += records_per_thread;
                }

                if ((batch_num + 1) % batch_interval == 0) {
                    double partial_train_error = total_train_error / total_train_records;
                    printf("epoch: %d file: %d batch %d/%d train error: %.8f learn rate: %.8f decay rate: %.8f\r", 
                        epoch, data_file_index + 1, batch_num + 1, (int)batch_count, partial_train_error, LEARN_RATE, DECAY_RATE);
                    fflush(stdout);
                }
                tnn_apply_gradients(&network, &gradient);
            }
            printf("\n");

            double data_file_error = total_data_file_error / total_data_file_records;
            printf("\tData File Error: %.8f (%.0f records).\n", data_file_error, total_data_file_records);
        }
        printf("\n");

        double training_error = total_train_error / total_train_records;

        double validate_error = tnn_validate();
        tnn_export_h(&network, validate_error, epoch);
        if (epoch % 10 == 0) tnn_export_bin(&network, validate_error, epoch);
        
        printf("training error: %.8f validate error: %.8f lowest error: %.8f\n", training_error, validate_error, lowest_error);
        printf("learn rate: %.8f decay rate: %.8f min learn rate: %.8f\n", LEARN_RATE, DECAY_RATE, LEARN_MIN);
        if (epoch == 1) {
            lowest_error = validate_error;
        }
        if (validate_error > lowest_error && LEARN_RATE > LEARN_MIN) {
            LEARN_RATE *= DECAY_RATE;
            if (LEARN_RATE < LEARN_MIN) LEARN_RATE = LEARN_MIN;
            printf(" ===> changed learn rate: %.8f\n", LEARN_RATE);
        }
        if (validate_error < lowest_error) {
            lowest_error = validate_error;
        }

        unsigned elapsed = util_get_time() - start_time;
        printf("time per epoch: %d seconds\n", (int)(elapsed / 1000));
        printf("------------------------------------------------------------------------------------------------------------\n\n");
    }
}

void tnn_prepare_data(int lines_per_file)
{
    char    source_name[1000];
    FILE    *source_file = NULL;
    char    target_name[1000];
    FILE    *target_file = NULL;
    int     target_file_count = 0;
    int     target_line_count = lines_per_file + 1;
    char    line[1000];
#ifdef TNN_USE_PROC_FILE
    TSAMPLE sample;
#endif
    
    printf("preparing data from %s to %s (lines per file: %d)\n", TNN_TEMP_SOURCE, TNN_DATA_SOURCE, lines_per_file);

    for (int i = 0; i < MAX_FILES; i++) {
        sprintf(source_name, TNN_TEMP_SOURCE, i);
        source_file = fopen(source_name, "r");
        if (!source_file) continue;
        printf("  reading file '%s'\n", source_name);
        while (fgets(line, 1000, source_file) != NULL) {
#ifdef TNN_USE_PROC_FILE
            tnn_line2record(line, &sample);
            if (is_mate_score(sample.eval)) sample.eval /= 5;
#endif
            if (target_line_count >= lines_per_file) {
                if (target_file) fclose(target_file);
                target_file_count++;
                sprintf(target_name, TNN_DATA_SOURCE, target_file_count);
#if defined(TNN_USE_PROC_FILE)
                target_file = fopen(target_name, "wb");
#else
                target_file = fopen(target_name, "w");
#endif
                if (!target_file) {
                    printf("error creating file: %s\n", target_name);
                    exit(-1);
                }
                target_line_count = 0;
                printf("    saving lines to '%s'\n", target_name);
            }
#ifdef TNN_USE_PROC_FILE
            if (fwrite(&sample, sizeof(TSAMPLE), 1, target_file) != 1) {
                fprintf(stderr, "**** Error writing file %d\n", target_file_count);
            }
#else
            fprintf(target_file, "%s", line);
#endif
            target_line_count++;
        }
        fclose(source_file);
    }

    if (target_file) fclose(target_file);
}

void tnn_verify_data(void)
{
    char    source_name[1000];
    FILE    *source_file = NULL;
    char    target_name[1000];
    FILE    *target_file = NULL;
    char    line[1000];
    TSAMPLE source_sample;
    TSAMPLE target_sample;


    sprintf(source_name, TNN_TEMP_SOURCE, 1);
    source_file = fopen(source_name, "r");
    if (!source_file) {
        printf("cannot open file: %s\n", source_name);
        return;
    }
    sprintf(target_name, TNN_DATA_SOURCE, 1);
    target_file = fopen(target_name, "rb");
    if (!target_file) {
        printf("cannot open file: %s\n", target_name);
        fclose(source_file);
        return;
    }
    
    printf("verify data from %s to %s\n", source_name, target_name);

    int error = 0;
    int count = 0;
    while (fgets(line, 1000, source_file) != NULL) {
        tnn_line2record(line, &source_sample);
        if (is_mate_score(source_sample.eval)) source_sample.eval /= 5;
        if (fread(&target_sample, sizeof(TSAMPLE), 1, target_file) != 1) break;
        count++;
        if (strncmp((const char *)&source_sample, (const char *)&target_sample, sizeof(TSAMPLE))) {
            printf("error: difference in source/target samples\n");
            error++;
        }
    }
    printf("Count: %d, error: %d\n", count, error);
    fclose(source_file);
    fclose(target_file);
}

void tnn_prepare_menu()
{
    char resp[100];

    while (TRUE) {
        printf("NN data preparation menu\n\n");

        printf("Settings:\n");
        printf("\tPositions per file...: %d\n", TNN_POS_PER_FILE);
        printf("\tTemp source file mask: %s\n", TNN_TEMP_SOURCE);
        printf("\tTarget file mask.....: %s\n", TNN_DATA_SOURCE);
        printf("\n");

        printf("\t1. Prepare data files\n");
        printf("\tx. Exit\n\n");
        printf("\t--> ");
        fgets(resp, 100, stdin);
        printf("\n");
        if (!strncmp(resp, "1", 1)) tnn_prepare_data(TNN_POS_PER_FILE);
        if (!strncmp(resp, "x", 1)) break;
    }
}

void tnn_training_menu()
{
    char resp[100];
#ifdef _MSC_VER
    int epoch_total = 100;
    int batch_size = 1000;
#else
    int epoch_total = 1000;
    int batch_size = 20000;
#endif

    while (TRUE) {
        printf("NN training menu\n\n");

        printf("Settings:\n");
        printf("\tEpochs Total: %d\n", epoch_total);
        printf("\tBatch Size..: %d\n", batch_size);
        printf("\n");

        printf("\t1. Train network\n");
        printf("\tx. Exit\n\n");
        printf("\t--> ");
        fgets(resp, 100, stdin);
        printf("\n");
        if (!strncmp(resp, "1", 1)) tnn_train(epoch_total, batch_size);
        if (!strncmp(resp, "x", 1)) break;
    }
}

// EOF
