import unittest
import util
from util import utf8
from binascii import hexlify, unhexlify
from ctypes import create_string_buffer

class PBKDF2Case(object):
    def __init__(self, items):
        # Format: HMAC_SHA_TYPE, PASSWORD, SALT, COST, EXPECTED
        self.typ = int(items[0])
        assert self.typ in [256, 512]
        self.passwd = unhexlify(items[1])
        extra_salt_bytes = '00000000'
        self.salt, self.salt_len = util.make_cbuffer(items[2] + extra_salt_bytes)
        self.cost = int(items[3])
        self.expected, self.expected_len = util.make_cbuffer(items[4])


class PBKDF2Tests(unittest.TestCase):

    PBKDF2_HMAC_SHA256_LEN, PBKDF2_HMAC_SHA512_LEN = 32, 64

    def setUp(self):
        if not hasattr(self, 'pbkdf2_hmac_sha256'):
            util.bind_all(self, util.pbkdf2_funcs)
            self.cases = []
            with open(util.root_dir + 'src/data/pbkdf2_hmac_sha_vectors.txt', 'r') as f:
                for l in f.readlines():
                    l = l.strip()
                    if len(l) == 0 or l.startswith('#'):
                        continue
                    self.cases.append(PBKDF2Case(l.split(',')))


    def test_pbkdf2_hmac(self):

        # Some test vectors are nuts (e.g. 2097152 cost), so only run the
        # first few. set these to -1 to run the whole suite (only needed
        # when refactoring the impl)
        num_crazy_256, num_crazy_512 = 10, 10

        for case in self.cases:

            if case.typ == 256:
                fn = self.pbkdf2_hmac_sha256
                mult = self.PBKDF2_HMAC_SHA256_LEN
                if case.cost > 100:
                    if num_crazy_256 == 0:
                         continue
                    num_crazy_256 -= 1
            else:
                fn = self.pbkdf2_hmac_sha512
                mult = self.PBKDF2_HMAC_SHA512_LEN
                if case.cost > 100:
                    if num_crazy_512 == 0:
                        continue
                    num_crazy_512 -= 1

            out_buf, out_len = util.make_cbuffer('00' * case.expected_len)
            if case.expected_len % mult != 0:
                # We only support output multiples of the hmac length
                continue

            ret = fn(case.passwd, len(case.passwd), case.salt, case.salt_len,
                     case.cost, out_buf, out_len)

            self.assertEqual(ret, 0)
            self.assertEqual(out_buf, case.expected)


if __name__ == '__main__':
    unittest.main()