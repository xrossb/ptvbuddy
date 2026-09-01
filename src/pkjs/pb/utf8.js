// https://gist.github.com/Yaffle/5458286

/**
 * Encode a string into UTF-8 bytes.
 * @param {string} string
 */
function encode(string) {
  const bytes = [];

  let i = 0;
  while (i < string.length) {
    const codePoint = string.codePointAt(i) || 0;

    let c = 0;
    let bits = 0;
    if (codePoint <= 0x0000007f) {
      c = 0;
      bits = 0x00;
    } else if (codePoint <= 0x000007ff) {
      c = 6;
      bits = 0xc0;
    } else if (codePoint <= 0x0000ffff) {
      c = 12;
      bits = 0xe0;
    } else if (codePoint <= 0x001fffff) {
      c = 18;
      bits = 0xf0;
    }

    bytes.push(bits | (codePoint >> c));

    c -= 6;
    while (c >= 0) {
      bytes.push(0x80 | ((codePoint >> c) & 0x3f));
      c -= 6;
    }

    i += codePoint >= 0x10000 ? 2 : 1;
  }

  return bytes;
}

/**
 * Decode UTF-8 bytes to a string.
 * @param {number[]} bytes
 */
function decode(bytes) {
  let string = "";

  let i = 0;
  while (i < bytes.length) {
    let byte = bytes[i];

    let bytesNeeded = 0;
    let codePoint = 0;
    if (byte <= 0x7f) {
      bytesNeeded = 0;
      codePoint = byte & 0xff;
    } else if (byte <= 0xdf) {
      bytesNeeded = 1;
      codePoint = byte & 0x1f;
    } else if (byte <= 0xef) {
      bytesNeeded = 2;
      codePoint = byte & 0x0f;
    } else if (byte <= 0xf4) {
      bytesNeeded = 3;
      codePoint = byte & 0x07;
    }

    if (bytes.length - i - bytesNeeded > 0) {
      let k = 0;
      while (k < bytesNeeded) {
        byte = bytes[i + k + 1];
        codePoint = (codePoint << 6) | (byte & 0x3f);
        k += 1;
      }
    } else {
      codePoint = 0xfffd;
      bytesNeeded = bytes.length - i;
    }

    string += String.fromCodePoint(codePoint);
    i += bytesNeeded + 1;
  }

  return string;
}

module.exports = { encode, decode };
