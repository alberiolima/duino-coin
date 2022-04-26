/*
 *** BASED ***

   Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef BR_BEARSSL_SHA1_H__
#define BR_BEARSSL_SHA1_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_hash_class_ br_hash_class;
struct br_hash_class_ {
  /**
     \brief Size (in bytes) of the context structure appropriate for
     computing this hash function.
  */
  size_t context_size;

  /**
     \brief Descriptor word that contains information about the hash
     function.

     For each word `xxx` described below, use `BR_HASHDESC_xxx_OFF`
     and `BR_HASHDESC_xxx_MASK` to access the specific value, as
     follows:

         (hf->desc >> BR_HASHDESC_xxx_OFF) & BR_HASHDESC_xxx_MASK

     The defined elements are:

      - `ID`: the symbolic identifier for the function, as defined
        in [TLS](https://tools.ietf.org/html/rfc5246#section-7.4.1.4.1)
        (MD5 = 1, SHA-1 = 2,...).

      - `OUT`: hash output size, in bytes.

      - `STATE`: internal running state size, in bytes.

      - `LBLEN`: base-2 logarithm for the internal block size, as
        defined for HMAC processing (this is 6 for MD5, SHA-1, SHA-224
        and SHA-256, since these functions use 64-byte blocks; for
        SHA-384 and SHA-512, this is 7, corresponding to their
        128-byte blocks).

     The descriptor may contain a few other flags.
  */
  uint32_t desc;

  /**
     \brief Initialisation method.

     This method takes as parameter a pointer to a context area,
     that it initialises. The first field of the context is set
     to this vtable; other elements are initialised for a new hash
     computation.

     \param ctx   pointer to (the first field of) the context.
  */
  void (*init)(const br_hash_class **ctx);

  /**
     \brief Data injection method.

     The `len` bytes starting at address `data` are injected into
     the running hash computation incarnated by the specified
     context. The context is updated accordingly. It is allowed
     to have `len == 0`, in which case `data` is ignored (and could
     be `NULL`), and nothing happens.
     on the input data.

     \param ctx    pointer to (the first field of) the context.
     \param data   pointer to the first data byte to inject.
     \param len    number of bytes to inject.
  */
  void (*update)(const br_hash_class **ctx, const void *data, size_t len);

  /**
     \brief Produce hash output.

     The hash output corresponding to all data bytes injected in the
     context since the last `init()` call is computed, and written
     in the buffer pointed to by `dst`. The hash output size depends
     on the implemented hash function (e.g. 16 bytes for MD5).
     The context is _not_ modified by this call, so further bytes
     may be afterwards injected to continue the current computation.

     \param ctx   pointer to (the first field of) the context.
     \param dst   destination buffer for the hash output.
  */
  void (*out)(const br_hash_class *const *ctx, void *dst);

  /**
     \brief Get running state.

     This method saves the current running state into the `dst`
     buffer. What constitutes the "running state" depends on the
     hash function; for Merkle-Damgård hash functions (like
     MD5 or SHA-1), this is the output obtained after processing
     each block. The number of bytes injected so far is returned.
     The context is not modified by this call.

     \param ctx   pointer to (the first field of) the context.
     \param dst   destination buffer for the state.
     \return  the injected total byte length.
  */
  uint64_t (*state)(const br_hash_class *const *ctx, void *dst);

  /**
     \brief Set running state.

     This methods replaces the running state for the function.

     \param ctx     pointer to (the first field of) the context.
     \param stb     source buffer for the state.
     \param count   injected total byte length.
  */
  void (*set_state)(const br_hash_class **ctx,
                    const void *stb, uint64_t count);
};

#ifndef BR_DOXYGEN_IGNORE
#define BR_HASHDESC_ID(id)           ((uint32_t)(id) << BR_HASHDESC_ID_OFF)
#define BR_HASHDESC_ID_OFF           0
#define BR_HASHDESC_ID_MASK          0xFF

#define BR_HASHDESC_OUT(size)        ((uint32_t)(size) << BR_HASHDESC_OUT_OFF)
#define BR_HASHDESC_OUT_OFF          8
#define BR_HASHDESC_OUT_MASK         0x7F

#define BR_HASHDESC_STATE(size)      ((uint32_t)(size) << BR_HASHDESC_STATE_OFF)
#define BR_HASHDESC_STATE_OFF        15
#define BR_HASHDESC_STATE_MASK       0xFF

#define BR_HASHDESC_LBLEN(ls)        ((uint32_t)(ls) << BR_HASHDESC_LBLEN_OFF)
#define BR_HASHDESC_LBLEN_OFF        23
#define BR_HASHDESC_LBLEN_MASK       0x0F

#define BR_HASHDESC_MD_PADDING       ((uint32_t)1 << 28)
#define BR_HASHDESC_MD_PADDING_128   ((uint32_t)1 << 29)
#define BR_HASHDESC_MD_PADDING_BE    ((uint32_t)1 << 30)
#endif

/**
   \brief Symbolic identifier for SHA-1.
*/
#define br_sha1_ID     2

/**
   \brief SHA-1 output size (in bytes).
*/
#define br_sha1_SIZE   20

/**
   \brief Constant vtable for SHA-1.
*/
extern const br_hash_class br_sha1_vtable;

/**
   \brief SHA-1 context.

   First field is a pointer to the vtable; it is set by the initialisation
   function. Other fields are not supposed to be accessed by user code.
*/
typedef struct {
  /**
     \brief Pointer to vtable for this context.
  */
  const br_hash_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
  unsigned char buf[64];
  uint64_t count;
  uint32_t val[5];
#endif
} br_sha1_context;

/**
   \brief SHA-1 context initialisation.

   This function initialises or resets a context for a new SHA-1
   computation. It also sets the vtable pointer.

   \param ctx   pointer to the context structure.
*/
void br_sha1_init(br_sha1_context *ctx);

/**
   \brief Inject some data bytes in a running SHA-1 computation.

   The provided context is updated with some data bytes. If the number
   of bytes (`len`) is zero, then the data pointer (`data`) is ignored
   and may be `NULL`, and this function does nothing.

   \param ctx    pointer to the context structure.
   \param data   pointer to the injected data.
   \param len    injected data length (in bytes).
*/
void br_sha1_update(br_sha1_context *ctx, const void *data, size_t len);

/**
   \brief Compute SHA-1 output.

   The SHA-1 output for the concatenation of all bytes injected in the
   provided context since the last initialisation or reset call, is
   computed and written in the buffer pointed to by `out`. The context
   itself is not modified, so extra bytes may be injected afterwards
   to continue that computation.

   \param ctx   pointer to the context structure.
   \param out   destination buffer for the hash output.
*/
void br_sha1_out(const br_sha1_context *ctx, void *out);

/**
   \brief Save SHA-1 running state.

   The running state for SHA-1 (output of the last internal block
   processing) is written in the buffer pointed to by `out`. The
   number of bytes injected since the last initialisation or reset
   call is returned. The context is not modified.

   \param ctx   pointer to the context structure.
   \param out   destination buffer for the running state.
   \return  the injected total byte length.
*/
uint64_t br_sha1_state(const br_sha1_context *ctx, void *out);

/**
   \brief Restore SHA-1 running state.

   The running state for SHA-1 is set to the provided values.

   \param ctx     pointer to the context structure.
   \param stb     source buffer for the running state.
   \param count   the injected total byte length.
*/
void br_sha1_set_state(br_sha1_context *ctx, const void *stb, uint64_t count);

#ifdef __cplusplus
}
#endif

#endif
