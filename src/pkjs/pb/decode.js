const type = require("./type");

class Reader {
  /**
   * @param {number[]} msg
   */
  constructor(msg) {
    this._buf = new Uint8Array(msg);
    this._pos = 0;
    this.field = 0;
    this.type = 0;
    this.data = new Uint8Array();
  }

  next() {
    if (this._pos >= this._buf.length) {
      return false;
    }

    const tlv = Number(this._readVarint());
    this.type = tlv & 0b111;
    this.field = tlv >>> 3;

    const start = this._pos;
    let len;
    switch (this.type) {
      case type.varint:
        this._skipVarint();
        this.data = this._buf.slice(start, this._pos);
        break;
      case type.i64:
        this._pos += 8;
        this.data = this._buf.slice(start, this._pos);
        break;
      case type.len:
        len = Number(this._readVarint());
        this._pos += len;
        this.data = this._buf.slice(start, this._pos);
        break;
      case type.i32:
        this._pos += 4;
        this.data = this._buf.slice(start, this._pos);
        break;
      default:
        this.data = new Uint8Array();
        throw new Error(`Unsupported wire type: ${this.type}`);
    }

    return true;
  }

  _readVarint() {
    let val = BigInt(0);

    let byte = 0x80;
    let i = 0;
    while (byte & 0x80) {
      if (i >= 10) {
        throw new Error("Read varint longer than 10 bytes.");
      }

      byte = this._buf[this._pos];
      val |= BigInt(byte << (7 * i));
      this._pos += 1;
      i += 1;
    }

    return val;
  }

  _skipVarint() {
    while (this._buf[this._pos] & 0x80) {
      this._pos += 1;
    }
    this._pos += 1;
  }
}

module.exports = { Reader };
