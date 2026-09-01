const type = require("./type");
const utf8 = require("./utf8");
const { Uint8Vec } = require("./vec");

/** Encodes values into a protobuf message. */
class Builder {
  constructor() {
    this._buffer = new Uint8Vec();
  }

  /** Return a JS array containing the encoded protobuf message. */
  build() {
    return this._buffer.toArray();
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  int32(field, value) {
    const i32 = new Int32Array([value]);
    const bytes = new Uint8Array(i32.buffer);
    Encode.field(this._buffer, field, type.varint);
    Encode.varint(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  int64(field, value) {
    const i64 = new BigInt64Array([value]);
    const bytes = new Uint8Array(i64.buffer);
    Encode.field(this._buffer, field, type.varint);
    Encode.varint(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  uint32(field, value) {
    const u32 = new Uint32Array([value]);
    const bytes = new Uint8Array(u32.buffer);
    Encode.field(this._buffer, field, type.varint);
    Encode.varint(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  uint64(field, value) {
    const u64 = new BigUint64Array([value]);
    const bytes = new Uint8Array(u64.buffer);
    Encode.field(this._buffer, field, type.varint);
    Encode.varint(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  sint32(field, value) {
    value = (value << 1) ^ (value >> 63);
    return this.int32(field, value);
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  sint64(field, value) {
    value = (value << BigInt(1)) ^ (value >> BigInt(63));
    return this.int64(field, value);
  }

  /**
   * @param {number} field
   * @param {boolean} value
   */
  bool(field, value) {
    return this.int32(field, value ? 1 : 0);
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  enum(field, value) {
    return this.int32(field, value);
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  fixed64(field, value) {
    const i64 = new BigInt64Array([value]);
    const bytes = new Uint8Array(i64.buffer);
    Encode.field(this._buffer, field, type.i64);
    Encode.i64(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  sfixed64(field, value) {
    value = (value << BigInt(1)) ^ (value >> BigInt(63));
    return this.fixed64(field, value);
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  double(field, value) {
    const f64 = new Float64Array([value]);
    const bytes = new Uint8Array(f64.buffer);
    Encode.field(this._buffer, field, type.i64);
    Encode.i64(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {string} value
   */
  string(field, value) {
    const bytes = utf8.encode(value);
    return this.bytes(field, bytes);
  }

  /**
   * @param {number} field
   * @param {ArrayLike<number>} value
   */
  bytes(field, value) {
    Encode.field(this._buffer, field, type.len);
    Encode.len(this._buffer, value);
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  fixed32(field, value) {
    const i32 = new Int32Array([value]);
    const bytes = new Uint8Array(i32.buffer);
    Encode.field(this._buffer, field, type.i32);
    Encode.i32(this._buffer, bytes);
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  sfixed32(field, value) {
    value = (value << 1) ^ (value >> 31);
    return this.fixed32(field, value);
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  float(field, value) {
    const f32 = new Float32Array([value]);
    const bytes = new Uint8Array(f32.buffer);
    Encode.field(this._buffer, field, type.i32);
    Encode.i32(this._buffer, bytes);
    return this;
  }
}

/** Binary encoders for various wire types. */
const Encode = {
  /**
   * Encodes a field header onto the buffer.
   * @param {Uint8Vec} vec
   * @param {number} id
   * @param {number} type
   */
  field(vec, id, type) {
    const tlv = (id << 3) | type;
    const u32 = new Uint32Array([tlv]);
    const bytes = new Uint8Array(u32.buffer);
    Encode.varint(vec, bytes);
  },

  /**
   * Encodes a varint onto the buffer.
   * @param {Uint8Vec} vec
   * @param {Uint8Array} bytes
   */
  varint(vec, bytes) {
    if (bytes.length > 8) {
      throw new Error("Encode.varint does not support > 64-bit values.");
    }

    vec.reserve(10);

    let val = BigInt(0);
    for (let i = 0; i < bytes.length; i++) {
      val |= BigInt(bytes[i]) << (BigInt(8) * BigInt(i));
    }

    while (val >= 0x80) {
      const byte = (val & BigInt(0x7f)) | BigInt(0x80);
      vec.append(Number(byte));
      val >>= BigInt(7);
    }

    vec.append(Number(val));
  },

  /**
   * Encodes a 64-bit value onto the buffer.
   * @param {Uint8Vec} vec
   * @param {Uint8Array} bytes
   */
  i64(vec, bytes) {
    if (bytes.length !== 8) {
      throw new Error("Encode.i64 requires a 64-bit value.");
    }

    vec.concat(bytes);
  },

  /**
   * Encodes a length-delimited record onto the buffer.
   * @param {Uint8Vec} vec
   * @param {ArrayLike<number>} bytes
   */
  len(vec, bytes) {
    const i32 = new Int32Array([bytes.length]);
    const lenBytes = new Uint8Array(i32.buffer);
    Encode.varint(vec, lenBytes);
    vec.concat(bytes);
  },

  /**
   * Encodes a 32-bit value onto the buffer.
   * @param {Uint8Vec} vec
   * @param {Uint8Array} bytes
   */
  i32(vec, bytes) {
    if (bytes.length !== 4) {
      throw new Error("Encode.i32 requires a 32-bit value.");
    }

    vec.concat(bytes);
  },
};

module.exports = { Builder };
