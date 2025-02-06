/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "transmuter_def.h"

/*Lookup table mapping int ids to their corresponding transmuter bits in (int_id, transmuter bit) pairs*/
const transmuter_pin_pair_t transmuter_interrupt_lookup[TRANSMUTER_INTERRUPT_PINS] = {
	{ 76,  0   },
	{ 77,  1   },
	{ 79,  2   },
	{ 91,  3   },
	{ 92,  4   },
	{ 93,  5   },
	{ 94,  6   },
	{ 95,  7   },
	{ 96,  8   },
	{ 97,  9   },
	{ 98,  10  },
	{ 99,  11  },
	{ 100, 12  },
	{ 101, 13  },
	{ 102, 14  },
	{ 103, 15  },
	{ 104, 16  },
	{ 105, 17  },
	{ 106, 18  },
	{ 107, 19  },
	{ 108, 20  },
	{ 109, 21  },
	{ 110, 22  },
	{ 111, 23  },
	{ 112, 24  },
	{ 113, 25  },
	{ 114, 26  },
	{ 115, 27  },
	{ 116, 28  },
	{ 117, 29  },
	{ 118, 30  },
	{ 119, 31  },
	{ 128, 32  },
	{ 129, 33  },
	{ 130, 34  },
	{ 131, 35  },
	{ 132, 36  },
	{ 133, 37  },
	{ 134, 38  },
	{ 135, 39  },
	{ 136, 40  },
	{ 137, 41  },
	{ 138, 42  },
	{ 139, 43  },
	{ 140, 44  },
	{ 141, 45  },
	{ 142, 46  },
	{ 143, 47  },
	{ 144, 48  },
	{ 145, 49  },
	{ 146, 50  },
	{ 147, 51  },
	{ 148, 52  },
	{ 149, 53  },
	{ 150, 54  },
	{ 151, 55  },
	{ 152, 56  },
	{ 153, 57  },
	{ 154, 58  },
	{ 155, 59  },
	{ 156, 60  },
	{ 157, 61  },
	{ 158, 62  },
	{ 159, 63  },
	{ 160, 64  },
	{ 161, 65  },
	{ 169, 66  },
	{ 170, 67  },
	{ 171, 68  },
	{ 172, 69  },
	{ 173, 70  },
	{ 174, 71  },
	{ 175, 72  },
	{ 176, 73  },
	{ 177, 74  },
	{ 178, 75  },
	{ 179, 76  },
	{ 180, 77  },
	{ 189, 78  },
	{ 190, 79  },
	{ 192, 80  },
	{ 193, 81  },
	{ 194, 82  },
	{ 195, 83  },
	{ 196, 84  },
	{ 197, 85  },
	{ 198, 86  },
	{ 199, 87  },
	{ 200, 88  },
	{ 201, 89  },
	{ 202, 90  },
	{ 203, 91  },
	{ 204, 92  },
	{ 205, 93  },
	{ 206, 94  },
	{ 207, 95  },
	{ 208, 96  },
	{ 209, 97  },
	{ 210, 98  },
	{ 211, 99  },
	{ 212, 100 },
	{ 213, 101 },
	{ 214, 102 },
	{ 215, 103 },
	{ 216, 104 },
	{ 217, 105 },
	{ 218, 106 },
	{ 219, 107 },
	{ 220, 108 },
	{ 221, 109 },
	{ 222, 110 },
	{ 223, 111 },
	{ 230, 112 },
	{ 231, 113 },
	{ 233, 114 },
	{ 256, 115 },
	{ 257, 116 },
	{ 258, 117 },
	{ 259, 118 },
	{ 260, 119 },
	{ 261, 120 },
	{ 262, 121 },
	{ 263, 122 },
	{ 272, 123 },
	{ 273, 124 },
	{ 274, 125 },
	{ 275, 126 },
	{ 276, 127 },
	{ 277, 128 },
	{ 278, 129 },
	{ 279, 130 },
	{ 280, 131 },
	{ 281, 132 },
	{ 282, 133 },
	{ 283, 134 },
	{ 306, 135 },
	{ 307, 136 },
	{ 308, 137 },
	{ 312, 138 },
	{ 313, 139 },
	{ 333, 140 },
	{ 334, 141 },
	{ 336, 142 },
	{ 337, 143 },
	{ 338, 144 },
	{ 339, 145 },
	{ 340, 146 },
	{ 341, 147 },
	{ 342, 148 },
	{ 343, 149 },
	{ 344, 150 },
	{ 345, 151 },
	{ 346, 152 },
	{ 347, 153 },
	{ 348, 154 },
	{ 349, 155 },
	{ 350, 156 },
	{ 351, 157 },
	{ 352, 158 },
	{ 353, 159 },
	{ 354, 160 },
	{ 355, 161 },
	{ 356, 162 },
	{ 357, 163 },
	{ 358, 164 },
	{ 359, 165 },
	{ 360, 166 },
	{ 361, 167 },
	{ 362, 168 },
	{ 363, 169 },
	{ 364, 170 },
	{ 365, 171 },
	{ 366, 172 },
	{ 367, 173 },
	{ 368, 174 },
	{ 407, 175 },
	{ 412, 176 },
	{ 413, 177 },
	{ 439, 178 },
	{ 440, 179 },
	{ 441, 180 },
	{ 442, 181 },
	{ 443, 182 },
	{ 444, 183 },
	{ 445, 184 },
	{ 446, 185 },
	{ 537, 186 },
	{ 538, 187 },
	{ 567, 188 },
	{ 570, 189 },
	{ 571, 190 },
	{ 791, 191 },
	{ 791, 192 },
	{ 791, 193 },
	{ 791, 194 },
	{ 792, 195 },
	{ 792, 196 },
	{ 792, 197 },
	{ 792, 198 },
	{ 793, 199 },
	{ 793, 200 },
	{ 793, 201 },
	{ 793, 202 },
	{ 794, 203 },
	{ 794, 204 },
	{ 794, 205 },
	{ 794, 206 },
	{ 795, 207 },
	{ 795, 208 },
	{ 795, 209 },
	{ 795, 210 },
	{ 796, 211 },
	{ 796, 212 },
	{ 796, 213 },
	{ 796, 214 },
	{ 797, 215 },
	{ 797, 216 },
	{ 797, 217 },
	{ 797, 218 },
	{ 798, 219 },
	{ 798, 220 },
	{ 798, 221 },
	{ 798, 222 },
	{ 799, 223 },
	{ 799, 224 },
	{ 799, 225 },
	{ 799, 226 },
	{ 800, 227 },
	{ 800, 228 },
	{ 800, 229 },
	{ 800, 230 },
	{ 801, 231 },
	{ 801, 232 },
	{ 801, 233 },
	{ 801, 234 },
	{ 802, 235 },
	{ 802, 236 },
	{ 802, 237 },
	{ 802, 238 },
	{ 803, 239 },
	{ 804, 240 },
	{ 805, 241 },
	{ 806, 242 },
	{ 815, 243 },
	{ 816, 244 },
	{ 817, 245 },
	{ 818, 246 },
	{ 819, 247 },
	{ 820, 248 },
	{ 821, 249 },
	{ 822, 250 },
	{ 823, 251 },
	{ 823, 252 },
	{ 823, 253 },
	{ 823, 254 },
	{ 824, 255 },
	{ 824, 256 },
	{ 824, 257 },
	{ 824, 258 },
	{ 825, 259 },
	{ 825, 260 },
	{ 825, 261 },
	{ 825, 262 },
	{ 826, 263 },
	{ 826, 264 },
	{ 826, 265 },
	{ 826, 266 },
	{ 827, 267 },
	{ 827, 268 },
	{ 827, 269 },
	{ 827, 270 },
	{ 828, 271 },
	{ 828, 272 },
	{ 828, 273 },
	{ 828, 274 },
	{ 829, 275 },
	{ 829, 276 },
	{ 829, 277 },
	{ 829, 278 },
	{ 830, 279 },
	{ 830, 280 },
	{ 830, 281 },
	{ 830, 282 },
	{ 831, 283 },
	{ 831, 284 },
	{ 831, 285 },
	{ 831, 286 },
	{ 832, 287 },
	{ 832, 288 },
	{ 832, 289 },
	{ 832, 290 },
	{ 833, 291 },
	{ 833, 292 },
	{ 833, 293 },
	{ 833, 294 },
	{ 834, 295 },
	{ 834, 296 },
	{ 834, 297 },
	{ 834, 298 },
	{ 835, 299 },
	{ 835, 300 },
	{ 835, 301 },
	{ 835, 302 },
	{ 836, 303 },
	{ 836, 304 },
	{ 836, 305 },
	{ 836, 306 },
	{ 837, 307 },
	{ 837, 308 },
	{ 837, 309 },
	{ 837, 310 },
	{ 838, 311 },
	{ 838, 312 },
	{ 838, 313 },
	{ 838, 314 },
	{ 839, 315 },
	{ 839, 316 },
	{ 839, 317 },
	{ 839, 318 },
	{ 842, 319 },
	{ 859, 320 },
	{ 860, 321 },
	{ 861, 322 },
	{ 862, 323 },
	{ 863, 324 },
	{ 864, 325 },
	{ 865, 326 },
	{ 866, 327 },
	{ 867, 328 },
	{ 868, 329 },
	{ 869, 330 },
	{ 870, 331 },
};

/*Lookup table mapping TRU ids to their corresponding transmuter bits in (int_id, transmuter bit) pairs*/
const transmuter_pin_pair_t transmuter_tru_lookup[TRANSMUTER_TRU_PINS] = {
	{ 96,  350 },
	{ 97,  351 },
	{ 98,  352 },
	{ 99,  353 },
	{ 100, 354 },
	{ 101, 355 },
	{ 102, 356 },
	{ 103, 357 },
	{ 104, 358 },
	{ 105, 359 },
	{ 106, 360 },
	{ 107, 361 },
	{ 108, 362 },
	{ 109, 363 },
	{ 110, 364 },
	{ 111, 365 },
	{ 336, 365 },
	{ 337, 366 },
	{ 338, 367 },
	{ 339, 368 },
	{ 340, 369 },
	{ 341, 370 },
	{ 342, 371 },
	{ 343, 372 },
	{ 344, 373 },
	{ 345, 374 },
	{ 346, 375 },
	{ 347, 376 },
	{ 348, 377 },
	{ 349, 378 },
	{ 350, 379 },
	{ 351, 380 },
	{ 352, 381 },
	{ 353, 382 },
	{ 354, 383 },
	{ 355, 384 },
	{ 356, 385 },
	{ 357, 386 },
	{ 358, 387 },
	{ 359, 388 },
	{ 360, 389 },
	{ 361, 390 },
	{ 362, 391 },
	{ 363, 392 },
	{ 364, 393 },
	{ 365, 394 },
	{ 366, 395 },
	{ 367, 396 },
};
