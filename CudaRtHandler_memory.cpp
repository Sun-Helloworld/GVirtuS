#include <cuda_runtime_api.h>
#include "CudaUtil.h"
#include "CudaRtHandler.h"

CUDA_ROUTINE_HANDLER(Free) {
    /* cudaError_t cudaFree(void *devPtr) */
    char *dev_ptr_handler =
            input_buffer->Assign<char>(CudaUtil::MarshaledDevicePointerSize);
    void *devPtr = pThis->GetDevicePointer(dev_ptr_handler);

    cudaError_t exit_code = cudaFree(devPtr);
    pThis->UnregisterDevicePointer(dev_ptr_handler);

    return new Result(exit_code);
}

CUDA_ROUTINE_HANDLER(Malloc) {
    /* cudaError_t cudaMalloc(void **devPtr, size_t size) */
    void *devPtr = NULL;
    char *dev_ptr_handler =
            input_buffer->Assign<char>(CudaUtil::MarshaledDevicePointerSize);
    size_t size = input_buffer->Get<size_t > ();

    cudaError_t exit_code = cudaMalloc(&devPtr, size);
    pThis->RegisterDevicePointer(dev_ptr_handler, devPtr, size);

    return new Result(exit_code);
}

CUDA_ROUTINE_HANDLER(Memcpy) {
    /* cudaError_t cudaError_t cudaMemcpy(void *dst, const void *src,
        size_t count, cudaMemcpyKind kind) */
    void *dst = NULL;
    void *src = NULL;

    cudaMemcpyKind kind = input_buffer->BackGet<cudaMemcpyKind > ();
    size_t count = input_buffer->BackGet<size_t > ();
    char *dev_ptr_handler;
    cudaError_t exit_code;
    Result * result = NULL;
    Buffer *out;

    switch (kind) {
        case cudaMemcpyHostToHost:
            // This should never happen
            result = NULL;
            break;
        case cudaMemcpyHostToDevice:
            dev_ptr_handler =
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            dst = pThis->GetDevicePointer(dev_ptr_handler);
            /* Achtung: this isn't strictly correct because here we assign just
             * a pointer to one character, any successive assign should
             * take inaxpectated result ... but it works here!
             */
            src = input_buffer->Assign<char>();
            exit_code = cudaMemcpy(dst, src, count, kind);
            result = new Result(exit_code);
            break;
        case cudaMemcpyDeviceToHost:
            // FIXME: use buffer delegate
            dst = new char[count];
            /* skipping a char for fake host pointer */
            input_buffer->Assign<char>();
            dev_ptr_handler = 
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            src = pThis->GetDevicePointer(dev_ptr_handler);
            exit_code = cudaMemcpy(dst, src, count, kind);
            out = new Buffer();
            out->Add<char>((char *) dst, count);
            delete[] (char *) dst;
            result = new Result(exit_code, out);
            break;
        case cudaMemcpyDeviceToDevice:
            dev_ptr_handler = 
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            dst = pThis->GetDevicePointer(dev_ptr_handler);
            dev_ptr_handler = 
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            src = pThis->GetDevicePointer(dev_ptr_handler);
            exit_code = cudaMemcpy(dst, src, count, kind);
            result = new Result(exit_code);
            break;
    }
    return result;
}

CUDA_ROUTINE_HANDLER(MemcpyAsync) {
    void *dst = NULL;
    void *src = NULL;
    
    cudaStream_t stream = input_buffer->BackGet<cudaStream_t>();
    cudaMemcpyKind kind = input_buffer->BackGet<cudaMemcpyKind > ();
    size_t count = input_buffer->BackGet<size_t > ();
    char *dev_ptr_handler;
    cudaError_t exit_code;
    Buffer * out;
    Result * result = NULL;
    switch (kind) {
        case cudaMemcpyHostToHost:
            result = new Result(cudaSuccess);
            break;
        case cudaMemcpyHostToDevice:
            dev_ptr_handler =
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            dst = pThis->GetDevicePointer(dev_ptr_handler);
            /* Achtung: this isn't strictly correct because here we assign just
             * a pointer to one character, any successive assign should
             * take inaxpectated result ... but it works here!
             */
            src = input_buffer->Assign<char>();
            exit_code = cudaMemcpy(dst, src, count, kind);
            result = new Result(exit_code);
            break;
        case cudaMemcpyDeviceToHost:
            // FIXME: use buffer delegate
            dst = new char[count];
            /* skipping a char for fake host pointer */
            input_buffer->Assign<char>();
            dev_ptr_handler =
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            src = pThis->GetDevicePointer(dev_ptr_handler);
            exit_code = cudaMemcpy(dst, src, count, kind);
            out = new Buffer();
            out->Add<char>((char *) dst, count);
            delete[] (char *) dst;
            result = new Result(exit_code, out);
            break;
        case cudaMemcpyDeviceToDevice:
            dev_ptr_handler =
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            dst = pThis->GetDevicePointer(dev_ptr_handler);
            dev_ptr_handler =
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            src = pThis->GetDevicePointer(dev_ptr_handler);
            exit_code = cudaMemcpyAsync(dst, src, count, kind, stream);
            result = new Result(exit_code);
            break;
    }
    return result;
}

CUDA_ROUTINE_HANDLER(MemcpyFromSymbol) {
    void *dst = NULL;

    char *dst_handler = input_buffer->Assign<char>(CudaUtil::MarshaledDevicePointerSize);
    char *symbol_handler = input_buffer->AssignString();
    char *symbol = input_buffer->AssignString();
    size_t count = input_buffer->Get<size_t > ();
    size_t offset = input_buffer->Get<size_t>();
    cudaMemcpyKind kind = input_buffer->Get<cudaMemcpyKind > ();

    char *our_symbol = const_cast<char *>(pThis->GetVar(symbol_handler));
    if(our_symbol != NULL)
        symbol = const_cast<char *>(our_symbol);

    cudaError_t exit_code;
    Result * result = NULL;
    Buffer * out = NULL;

    switch (kind) {
        case cudaMemcpyHostToHost:
            // This should never happen
            result = new Result(cudaErrorInvalidMemcpyDirection);
            break;
        case cudaMemcpyHostToDevice:
            // This should never happen
            result = new Result(cudaErrorInvalidMemcpyDirection);
            break;
        case cudaMemcpyDeviceToHost:
            out = new Buffer(count);
            dst = out->Delegate<char>(count);
            exit_code = cudaMemcpyFromSymbol(dst, symbol, count, offset, kind);
            result = new Result(exit_code, out);
            break;
        case cudaMemcpyDeviceToDevice:
            dst = pThis->GetDevicePointer(dst_handler);
            exit_code = cudaMemcpyFromSymbol(dst, symbol, count, offset, kind);
            result = new Result(exit_code);
            break;
    }
    return result;
}

CUDA_ROUTINE_HANDLER(MemcpyToSymbol) {
    void *src = NULL;

    cudaMemcpyKind kind = input_buffer->BackGet<cudaMemcpyKind > ();
    size_t offset = input_buffer->BackGet<size_t>();
    size_t count = input_buffer->BackGet<size_t > ();
    char *handler = input_buffer->AssignString();
    char *symbol = input_buffer->AssignString();
    char *our_symbol = const_cast<char *>(pThis->GetVar(handler));
    if(our_symbol != NULL)
        symbol = const_cast<char *>(our_symbol);
    char *dev_ptr_handler;
    cudaError_t exit_code;
    Result * result = NULL;

    switch (kind) {
        case cudaMemcpyHostToHost:
            // This should never happen
            result = new Result(cudaErrorInvalidMemcpyDirection);
            break;
        case cudaMemcpyHostToDevice:
            /* Achtung: this isn't strictly correct because here we assign just
             * a pointer to one character, any successive assign should
             * take inaxpectated result ... but it works here!
             */
            src = input_buffer->Assign<char>();
            exit_code = cudaMemcpyToSymbol(symbol, src, count, offset, kind);
            result = new Result(exit_code);
            break;
        case cudaMemcpyDeviceToHost:
            // This should never happen
            result = new Result(cudaErrorInvalidMemcpyDirection);
            break;
        case cudaMemcpyDeviceToDevice:
            dev_ptr_handler =
                    input_buffer->Assign<char>(
                    CudaUtil::MarshaledDevicePointerSize);
            src = pThis->GetDevicePointer(dev_ptr_handler);
            exit_code = cudaMemcpyToSymbol(symbol, src, count, offset, kind);
            result = new Result(exit_code);
            break;
    }
    return result;
}

CUDA_ROUTINE_HANDLER(Memset) {
    char *dev_ptr_handler =
            input_buffer->Assign<char>(CudaUtil::MarshaledDevicePointerSize);
    void *devPtr = pThis->GetDevicePointer(dev_ptr_handler);
    int value = input_buffer->Get<int>();
    size_t count = input_buffer->Get<size_t>();

    cudaError_t exit_code = cudaMemset(devPtr, value, count);

    return new Result(exit_code);
}
