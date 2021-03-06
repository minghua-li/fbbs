'use strict';

(function() {
	var Util = window.Util = {
		escape: Mustache.escape
	};

	(function() {
		var COMBINING = [
			[ 0x0300, 0x036F ], [ 0x0483, 0x0489 ], [ 0x0591, 0x05BD ],
			[ 0x05BF, 0x05BF ], [ 0x05C1, 0x05C2 ], [ 0x05C4, 0x05C5 ],
			[ 0x05C7, 0x05C7 ], [ 0x0600, 0x0605 ], [ 0x0610, 0x061A ],
			[ 0x061C, 0x061C ], [ 0x064B, 0x065F ], [ 0x0670, 0x0670 ],
			[ 0x06D6, 0x06DD ], [ 0x06DF, 0x06E4 ], [ 0x06E7, 0x06E8 ],
			[ 0x06EA, 0x06ED ], [ 0x070F, 0x070F ], [ 0x0711, 0x0711 ],
			[ 0x0730, 0x074A ], [ 0x07A6, 0x07B0 ], [ 0x07EB, 0x07F3 ],
			[ 0x0816, 0x0819 ], [ 0x081B, 0x0823 ], [ 0x0825, 0x0827 ],
			[ 0x0829, 0x082D ], [ 0x0859, 0x085B ], [ 0x08E4, 0x0902 ],
			[ 0x093A, 0x093A ], [ 0x093C, 0x093C ], [ 0x0941, 0x0948 ],
			[ 0x094D, 0x094D ], [ 0x0951, 0x0957 ], [ 0x0962, 0x0963 ],
			[ 0x0981, 0x0981 ], [ 0x09BC, 0x09BC ], [ 0x09C1, 0x09C4 ],
			[ 0x09CD, 0x09CD ], [ 0x09E2, 0x09E3 ], [ 0x0A01, 0x0A02 ],
			[ 0x0A3C, 0x0A3C ], [ 0x0A41, 0x0A42 ], [ 0x0A47, 0x0A48 ],
			[ 0x0A4B, 0x0A4D ], [ 0x0A51, 0x0A51 ], [ 0x0A70, 0x0A71 ],
			[ 0x0A75, 0x0A75 ], [ 0x0A81, 0x0A82 ], [ 0x0ABC, 0x0ABC ],
			[ 0x0AC1, 0x0AC5 ], [ 0x0AC7, 0x0AC8 ], [ 0x0ACD, 0x0ACD ],
			[ 0x0AE2, 0x0AE3 ], [ 0x0B01, 0x0B01 ],	[ 0x0B3C, 0x0B3C ],
			[ 0x0B3F, 0x0B3F ], [ 0x0B41, 0x0B44 ], [ 0x0B4D, 0x0B4D ],
			[ 0x0B56, 0x0B56 ], [ 0x0B62, 0x0B63 ], [ 0x0B82, 0x0B82 ],
			[ 0x0BC0, 0x0BC0 ], [ 0x0BCD, 0x0BCD ], [ 0x0C00, 0x0C00 ],
			[ 0x0C3E, 0x0C40 ], [ 0x0C46, 0x0C48 ], [ 0x0C4A, 0x0C4D ],
			[ 0x0C55, 0x0C56 ], [ 0x0C62, 0x0C63 ], [ 0x0C81, 0x0C81 ],
			[ 0x0CBC, 0x0CBC ], [ 0x0CBF, 0x0CBF ], [ 0x0CC6, 0x0CC6 ],
			[ 0x0CCC, 0x0CCD ], [ 0x0CE2, 0x0CE3 ], [ 0x0D01, 0x0D01 ],
			[ 0x0D41, 0x0D44 ], [ 0x0D4D, 0x0D4D ], [ 0x0D62, 0x0D63 ],
			[ 0x0DCA, 0x0DCA ], [ 0x0DD2, 0x0DD4 ], [ 0x0DD6, 0x0DD6 ],
			[ 0x0E31, 0x0E31 ], [ 0x0E34, 0x0E3A ], [ 0x0E47, 0x0E4E ],
			[ 0x0EB1, 0x0EB1 ], [ 0x0EB4, 0x0EB9 ], [ 0x0EBB, 0x0EBC ],
			[ 0x0EC8, 0x0ECD ], [ 0x0F18, 0x0F19 ], [ 0x0F35, 0x0F35 ],
			[ 0x0F37, 0x0F37 ], [ 0x0F39, 0x0F39 ], [ 0x0F71, 0x0F7E ],
			[ 0x0F80, 0x0F84 ], [ 0x0F86, 0x0F87 ], [ 0x0F8D, 0x0F97 ],
			[ 0x0F99, 0x0FBC ], [ 0x0FC6, 0x0FC6 ], [ 0x102D, 0x1030 ],
			[ 0x1032, 0x1037 ], [ 0x1039, 0x103A ], [ 0x103D, 0x103E ],
			[ 0x1058, 0x1059 ], [ 0x105E, 0x1060 ], [ 0x1071, 0x1074 ],
			[ 0x1082, 0x1082 ], [ 0x1085, 0x1086 ], [ 0x108D, 0x108D ],
			[ 0x109D, 0x109D ], [ 0x1160, 0x11FF ], [ 0x135D, 0x135F ],
			[ 0x1712, 0x1714 ], [ 0x1732, 0x1734 ], [ 0x1752, 0x1753 ],
			[ 0x1772, 0x1773 ], [ 0x17B4, 0x17B5 ], [ 0x17B7, 0x17BD ],
			[ 0x17C6, 0x17C6 ], [ 0x17C9, 0x17D3 ], [ 0x17DD, 0x17DD ],
			[ 0x180B, 0x180E ], [ 0x18A9, 0x18A9 ], [ 0x1920, 0x1922 ],
			[ 0x1927, 0x1928 ], [ 0x1932, 0x1932 ], [ 0x1939, 0x193B ],
			[ 0x1A17, 0x1A18 ], [ 0x1A1B, 0x1A1B ], [ 0x1A56, 0x1A56 ],
			[ 0x1A58, 0x1A5E ], [ 0x1A60, 0x1A60 ], [ 0x1A62, 0x1A62 ],
			[ 0x1A65, 0x1A6C ], [ 0x1A73, 0x1A7C ], [ 0x1A7F, 0x1A7F ],
			[ 0x1AB0, 0x1ABE ], [ 0x1B00, 0x1B03 ], [ 0x1B34, 0x1B34 ],
			[ 0x1B36, 0x1B3A ], [ 0x1B3C, 0x1B3C ], [ 0x1B42, 0x1B42 ],
			[ 0x1B6B, 0x1B73 ], [ 0x1B80, 0x1B81 ], [ 0x1BA2, 0x1BA5 ],
			[ 0x1BA8, 0x1BA9 ], [ 0x1BAB, 0x1BAD ], [ 0x1BE6, 0x1BE6 ],
			[ 0x1BE8, 0x1BE9 ], [ 0x1BED, 0x1BED ], [ 0x1BEF, 0x1BF1 ],
			[ 0x1C2C, 0x1C33 ], [ 0x1C36, 0x1C37 ], [ 0x1CD0, 0x1CD2 ],
			[ 0x1CD4, 0x1CE0 ], [ 0x1CE2, 0x1CE8 ], [ 0x1CED, 0x1CED ],
			[ 0x1CF4, 0x1CF4 ], [ 0x1CF8, 0x1CF9 ], [ 0x1DC0, 0x1DF5 ],
			[ 0x1DFC, 0x1DFF ], [ 0x200B, 0x200F ], [ 0x202A, 0x202E ],
			[ 0x2060, 0x2064 ], [ 0x2066, 0x206F ], [ 0x20D0, 0x20F0 ],
			[ 0x2CEF, 0x2CF1 ], [ 0x2D7F, 0x2D7F ], [ 0x2DE0, 0x2DFF ],
			[ 0x302A, 0x302D ], [ 0x3099, 0x309A ], [ 0xA66F, 0xA672 ],
			[ 0xA674, 0xA67D ], [ 0xA69F, 0xA69F ], [ 0xA6F0, 0xA6F1 ],
			[ 0xA802, 0xA802 ], [ 0xA806, 0xA806 ], [ 0xA80B, 0xA80B ],
			[ 0xA825, 0xA826 ], [ 0xA8C4, 0xA8C4 ], [ 0xA8E0, 0xA8F1 ],
			[ 0xA926, 0xA92D ], [ 0xA947, 0xA951 ], [ 0xA980, 0xA982 ],
			[ 0xA9B3, 0xA9B3 ], [ 0xA9B6, 0xA9B9 ], [ 0xA9BC, 0xA9BC ],
			[ 0xA9E5, 0xA9E5 ], [ 0xAA29, 0xAA2E ], [ 0xAA31, 0xAA32 ],
			[ 0xAA35, 0xAA36 ], [ 0xAA43, 0xAA43 ], [ 0xAA4C, 0xAA4C ],
			[ 0xAA7C, 0xAA7C ], [ 0xAAB0, 0xAAB0 ], [ 0xAAB2, 0xAAB4 ],
			[ 0xAAB7, 0xAAB8 ], [ 0xAABE, 0xAABF ], [ 0xAAC1, 0xAAC1 ],
			[ 0xAAEC, 0xAAED ], [ 0xAAF6, 0xAAF6 ], [ 0xABE5, 0xABE5 ],
			[ 0xABE8, 0xABE8 ], [ 0xABED, 0xABED ], [ 0xFB1E, 0xFB1E ],
			[ 0xFE00, 0xFE0F ], [ 0xFE20, 0xFE2D ], [ 0xFEFF, 0xFEFF ],
			[ 0xFFF9, 0xFFFB ], [ 0x101FD, 0x101FD ], [ 0x102E0, 0x102E0 ],
			[ 0x10376, 0x1037A ], [ 0x10A01, 0x10A03 ], [ 0x10A05, 0x10A06 ],
			[ 0x10A0C, 0x10A0F ], [ 0x10A38, 0x10A3A ], [ 0x10A3F, 0x10A3F ],
			[ 0x10AE5, 0x10AE6 ], [ 0x11001, 0x11001 ], [ 0x11038, 0x11046 ],
			[ 0x1107F, 0x11081 ], [ 0x110B3, 0x110B6 ], [ 0x110B9, 0x110BA ],
			[ 0x110BD, 0x110BD ], [ 0x11100, 0x11102 ], [ 0x11127, 0x1112B ],
			[ 0x1112D, 0x11134 ], [ 0x11173, 0x11173 ], [ 0x11180, 0x11181 ],
			[ 0x111B6, 0x111BE ], [ 0x1122F, 0x11231 ], [ 0x11234, 0x11234 ],
			[ 0x11236, 0x11237 ], [ 0x112DF, 0x112DF ], [ 0x112E3, 0x112EA ],
			[ 0x11301, 0x11301 ], [ 0x1133C, 0x1133C ], [ 0x11340, 0x11340 ],
			[ 0x11366, 0x1136C ], [ 0x11370, 0x11374 ], [ 0x114B3, 0x114B8 ],
			[ 0x114BA, 0x114BA ], [ 0x114BF, 0x114C0 ], [ 0x114C2, 0x114C3 ],
			[ 0x115B2, 0x115B5 ], [ 0x115BC, 0x115BD ], [ 0x115BF, 0x115C0 ],
			[ 0x11633, 0x1163A ], [ 0x1163D, 0x1163D ], [ 0x1163F, 0x11640 ],
			[ 0x116AB, 0x116AB ], [ 0x116AD, 0x116AD ], [ 0x116B0, 0x116B5 ],
			[ 0x116B7, 0x116B7 ], [ 0x16AF0, 0x16AF4 ], [ 0x16B30, 0x16B36 ],
			[ 0x16F8F, 0x16F92 ], [ 0x1BC9D, 0x1BC9E ], [ 0x1BCA0, 0x1BCA3 ],
			[ 0x1D167, 0x1D169 ], [ 0x1D173, 0x1D182 ], [ 0x1D185, 0x1D18B ],
			[ 0x1D1AA, 0x1D1AD ], [ 0x1D242, 0x1D244 ], [ 0x1E8D0, 0x1E8D6 ],
			[ 0xE0001, 0xE0001 ], [ 0xE0020, 0xE007F ], [ 0xE0100, 0xE01EF ],
		],
		FULL_WIDTH = [
			[ 0x1100, 0x115F ], [ 0x2329, 0x232A ], [ 0x2E80, 0x2E99 ],
			[ 0x2E9B, 0x2EF3 ], [ 0x2F00, 0x2FD5 ], [ 0x2FF0, 0x2FFB ],
			[ 0x3000, 0x303E ], [ 0x3041, 0x3096 ], [ 0x3099, 0x30FF ],
			[ 0x3105, 0x312D ], [ 0x3131, 0x318E ], [ 0x3190, 0x31BA ],
			[ 0x31C0, 0x31E3 ], [ 0x31F0, 0x321E ], [ 0x3220, 0x3247 ],
			[ 0x3250, 0x32FE ], [ 0x3300, 0x4DBF ], [ 0x4E00, 0xA48C ],
			[ 0xA490, 0xA4C6 ], [ 0xA960, 0xA97C ], [ 0xAC00, 0xD7A3 ],
			[ 0xF900, 0xFAFF ], [ 0xFE10, 0xFE19 ], [ 0xFE30, 0xFE52 ],
			[ 0xFE54, 0xFE66 ], [ 0xFE68, 0xFE6B ], [ 0xFF01, 0xFF60 ],
			[ 0xFFE0, 0xFFE6 ], [ 0x1B000, 0x1B001 ], [ 0x1F200, 0x1F202 ],
			[ 0x1F210, 0x1F23A ], [ 0x1F240, 0x1F248 ], [ 0x1F250, 0x1F251 ],
			[ 0x20000, 0x2FFFD ], [ 0x30000, 0x3FFFD ]
		],
		FULL_WIDTH_GBK = [
			[ 164, 164 ], [ 167, 168 ], [ 176, 177 ], [ 183, 183 ], [ 215, 215 ],
			[ 224, 225 ], [ 232, 234 ], [ 236, 237 ], [ 242, 243 ], [ 247, 247 ],
			[ 249, 250 ], [ 252, 252 ], [ 257, 257 ], [ 275, 275 ], [ 283, 283 ],
			[ 299, 299 ], [ 324, 324 ], [ 328, 328 ], [ 333, 333 ], [ 363, 363 ],
			[ 462, 462 ], [ 464, 464 ], [ 466, 466 ], [ 468, 468 ], [ 470, 470 ],
			[ 472, 472 ], [ 474, 474 ], [ 476, 476 ], [ 593, 593 ], [ 609, 609 ],
			[ 711, 711 ], [ 713, 715 ], [ 729, 729 ], [ 913, 929 ], [ 931, 937 ],
			[ 945, 961 ], [ 963, 969 ], [ 1025, 1025 ], [ 1040, 1103 ],
			[ 1105, 1105 ], [ 8208, 8208 ], [ 8211, 8214 ], [ 8216, 8217 ],
			[ 8220, 8221 ], [ 8229, 8230 ], [ 8240, 8240 ], [ 8242, 8243 ],
			[ 8245, 8245 ], [ 8251, 8251 ], [ 8451, 8451 ], [ 8453, 8453 ],
			[ 8457, 8457 ], [ 8470, 8470 ], [ 8481, 8481 ], [ 8544, 8555 ],
			[ 8560, 8569 ], [ 8592, 8595 ], [ 8598, 8601 ], [ 8712, 8712 ],
			[ 8719, 8719 ], [ 8721, 8721 ], [ 8725, 8725 ], [ 8730, 8730 ],
			[ 8733, 8736 ], [ 8739, 8739 ], [ 8741, 8741 ], [ 8743, 8747 ],
			[ 8750, 8750 ], [ 8756, 8759 ], [ 8765, 8765 ], [ 8776, 8776 ],
			[ 8780, 8780 ], [ 8786, 8786 ], [ 8800, 8801 ], [ 8804, 8807 ],
			[ 8814, 8815 ], [ 8853, 8853 ], [ 8857, 8857 ], [ 8869, 8869 ],
			[ 8895, 8895 ], [ 8978, 8978 ], [ 9312, 9321 ], [ 9332, 9371 ],
			[ 9472, 9547 ], [ 9552, 9587 ], [ 9601, 9615 ], [ 9619, 9621 ],
			[ 9632, 9633 ], [ 9650, 9651 ], [ 9660, 9661 ], [ 9670, 9671 ],
			[ 9675, 9675 ], [ 9678, 9679 ], [ 9698, 9701 ], [ 9733, 9734 ],
			[ 9737, 9737 ], [ 9792, 9792 ], [ 9794, 9794 ]
		],

		contains = function(table, c) {
			var min = 0, mid, max = table.length - 1;
			if (c < table[0][0] || c > table[max][1]) return false;
			while (max >= min) {
				mid = Math.floor((min + max) / 2);
				if (c > table[mid][1])
					min = mid + 1;
				else if (c < table[mid][0])
					max = mid - 1;
				else
					return true;
			}
			return false;
		},

		charWidth = function(c) {
			if (c < 32 || (c >= 0x7f && c < 0xa0)) return 0;
			if (contains(COMBINING, c)) return 0;
			if (contains(FULL_WIDTH, c) || contains(FULL_WIDTH_GBK, c)) return 2;
			return 1;
		},

		iterate = function(str, callback) {
			var i, c, c2, length = str.length;
			for (i = 0; i < length; ++i) {
				c = str.charCodeAt(i);
				if (c >= 0xd800 && c <= 0xdbff) {
					c2 = str.charCodeAt(++i);
					if (!isNan(c2))
						c = (c - 0xd800) * 0x400 + (c2 - 0xdc00) + 0x10000;
				}
				if (callback(c, i) === false)
					break;
			}
		};

		var Width = Util.Width = {
			of: function(str) {
				var w = 0;
				iterate(str, function(c, i) {
					w += charWidth(c);
				});
				return w;
			}
		};

		Width.Seeker = function(str) {
			var begin = 0,
				len = str.length;
			this.next = function(width) {
				var s = str.substring(begin), end = len,
					ansi = false, code = '[0123456789;';

				if (len - begin < width || !s) {
					begin = len;
					return { str: s, width: Width.of(s) };
				}

				iterate(s, function(c, i) {
					if (ansi) {
						if (code.indexOf(c) < 0)
							ansi = false;
					} else {
						if (c == '\x1b') {
							ansi = true;
						} else {
							width -= charWidth(c);
							if (width < 0) {
								end = begin + i;
								return false;
							}
						}
					}
				});
				s = str.substring(begin, end);
				begin = end;
				return { str: s, width: width };
			}
		};
	})();

	(function() {
		var C = 4294967296;

		var Long = Util.Long = function(s) {
			var h = 0, l = 0, b = 0, e = 10, len;
			if (s < 4503599627370496) { // 2^52
				h = Math.floor(s / C);
				l = s % C;
			} else {
				len = s.length;
				while (e <= len) {
					l = l * 10 + parseInt(s.substring(b, e));
					if (l < C) {
						h *= 10;
					} else {
						h = h * 10 + Math.floor(l / C);
						l = l % C;
					}
					b = e;
					++e;
				}
			}
			this.h = h;
			this.l = l;
		};

		$.extend(Long.prototype, {
			rshift: function(n) {
				this.l = (this.l >>> n) + (this.h & ((1 << n) - 1)) * Math.pow(2, 32 - n);
				this.h >>= n;
				return this;
			},

			lshift: function(n) {
				this.h = (this.h << n) + (this.l >>> (32 - n));
				this.l <<= n;
				return this;
			},

			toInteger: function() {
				return this.h * C + this.l;
			},

			toString: function() {
				var r = [ 0, this.l ], i, j, h = '' + this.h, l = h.length, s, t,
					LEN = 10;
				if (this.h > 0) {
					for (i = 0; i < l; ++i) {
						t = C * h.charAt(i);
						if (t > 0) {
							s = '' + t;
							j = s.length + l - 1 - i - LEN;
							if (j > 0)
								r[0] += parseInt(s.substring(0, j));
							r[1] += parseInt(s.substring(j)) * Math.pow(10, l - 1 - i);
						}
					}
				}
				if (r[1] > 9999999999) {
					r[0] += Math.floor(r[1] / 1e10);
					r[1] = r[1] % 1e10;
				}
				return '' + (r[0] > 0 ? r[0] : '') + r[1];
			}
		});
	})();
})();

(function() {
	var relativeDate = function(d, n) {
		var	floor = Math.floor,
			diff = (n - d) / 1000;
		if (diff < 60)
			return '刚才';
		if (diff < 3600)
			return floor(diff / 60) + '分钟前';
		if (diff < 86400)
			return floor(diff / 3600) + '小时前';

		var today = n - (n - n.getTimezoneOffset() * 60000) % 86400000,
			min = d.getMinutes();
		if (min < 10)
			min = '0' + min;
		if (d > today - 86400000)
			return '昨天 ' + d.getHours() + ':' + min;
		if (d > today - 86400000 * 2)
			return '前天 ' + d.getHours() + ':' + min;
		if (diff < 86400 * 8)
			return floor(diff / 86400) + '天前';
		if (d.getFullYear() == n.getFullYear())
			return (d.getMonth() + 1) + '-' + d.getDate();
		return d.getFullYear() + '-' + (d.getMonth() + 1) + '-' + d.getDate();
	};

	App.hook('time', function(e) {
		var $e = $(e),
			now = new Date(),
			d = new Date($e.text());
		$e.attr('title', d.toLocaleString());
		$e.text(relativeDate(d, now));
	});
})();
