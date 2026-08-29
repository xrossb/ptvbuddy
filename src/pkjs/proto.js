/* eslint-disable no-unused-vars */

const wireType = {
  varint: 0,
  i64: 1,
  len: 2,
  i32: 5,
};

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
    Encode.field(this._buffer, field, wireType.varint);
    Encode.varint(this._buffer, value);
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  int64(field, value) {
    Encode.field(this._buffer, field, wireType.varint);
    Encode.varint(this._buffer, value);
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  uint32(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  uint64(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  sint32(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  sint64(field, value) {
    // TODO
    return this;
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
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {bigint} value
   */
  sfixed64(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  double(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {string} value
   */
  string(field, value) {
    const utf8 = new TextEncoder().encode(value);
    return this.bytes(field, utf8);
  }

  /**
   * @param {number} field
   * @param {Uint8Array} value
   */
  bytes(field, value) {
    Encode.field(this._buffer, field, wireType.len);
    Encode.len(this._buffer, value);
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  fixed32(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  sfixed32(field, value) {
    // TODO
    return this;
  }

  /**
   * @param {number} field
   * @param {number} value
   */
  float(field, value) {
    // TODO
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
    Encode.varint(vec, tlv);
  },

  /**
   * Encodes a varint onto the buffer.
   * @param {Uint8Vec} vec
   * @param {bigint} v
   */
  varint(vec, v) {
    // Varints are max 10 bytes.
    vec.reserve(10);

    for (let i = 0; i < 10; i++) {
      // Take 7 bits.
      let byte = v & 0x7f;
      v >>= 7;

      // Last bits; write without continuation bit and finish up.
      if (v <= 0) {
        vec.append(byte);
        break;
      }

      // More to come; set continuation bit.
      byte |= 0x80;
      vec.append(byte);
    }
  },

  /**
   * Encodes a 64-bit value onto the buffer.
   * @param {Uint8Vec} vec
   * @param {Uint8Array} v
   */
  i64(vec, v) {
    console.assert(v.length === 8, "Encode.i64 requires a 64-bit value.");
    vec.concat(v);
  },

  /**
   * Encodes a length-delimited record onto the buffer.
   * @param {Uint8Vec} vec
   * @param {Uint8Array} v
   */
  len(vec, v) {
    Encode.varint(vec, v.length);
    vec.concat(v);
  },

  /**
   * Encodes a 32-bit value onto the buffer.
   * @param {Uint8Vec} vec
   * @param {Uint8Array} v
   */
  i32(vec, v) {
    console.assert(v.length === 4, "Encode.i32 requires a 32-bit value.");
    vec.concat(v);
  },
};

/** Vector of bytes, backed by a dynamically sized Uint8Array. */
class Uint8Vec {
  constructor() {
    this._array = new Uint8Array();
    this._length = 0;
  }

  /** Current utilised length of the vector. */
  get length() {
    return this._length;
  }

  /** Total reserved capacity of the vector. */
  get capacity() {
    return this._array.length;
  }

  /** Create a plain JS array representation of the vector. */
  toArray() {
    return Array.from(this._array.slice(0, this._length));
  }

  /**
   * Append individual bytes to the end of the vector.
   * @param {...number} bytes
   */
  append(...bytes) {
    this.concat(bytes);
  }

  /**
   * Concatenate an array to the end of the vector.
   * @param {Uint8Array} array
   */
  concat(array) {
    this.reserve(array.length);
    this._array.set(array, this._length);
    this._length += array.length;
  }

  /**
   * Pre-allocate additional empty capacity in the vector.
   * @param {number} capacity
   */
  reserve(capacity) {
    if (this.capacity - this.length >= capacity) {
      // Already enough empty capacity.
      return;
    }

    const newCapacity = Math.max(this.capacity << 1, this.capacity + capacity);
    const newBuffer = this._array.buffer.transferToFixedLength(newCapacity);
    this._array = new Uint8Array(newBuffer);
  }
}

module.exports = {
  Builder,
};
