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
   * @param {ArrayLike<number>} array
   */
  concat(array) {
    this.reserve(array.length);
    this._array.set(array, this._length);
    this._length += array.length;
  }

  /**
   * Pre-allocate additional empty capacity in the vector.
   * @param {number} size
   */
  reserve(size) {
    const required = this.length + size;
    if (this.capacity >= required) {
      // Already enough empty capacity.
      return;
    }

    let newCapacity = this.capacity || 1;
    while (newCapacity < required) {
      newCapacity <<= 1;
    }

    const newBuffer = this._array.buffer.transferToFixedLength(newCapacity);
    this._array = new Uint8Array(newBuffer);
  }
}

module.exports = { Uint8Vec };
