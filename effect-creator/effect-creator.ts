interface IColor {
    r: number;
    g: number;
    b: number;
}
class Color implements Color {
    public r: number;
    public g: number;
    public b: number;

    public constructor(color: number);
    public constructor(r: number, g: number, b: number);
    public constructor(
        r: number,
        g: number = typeof r === "number" ? r : 0,
        b: number = typeof r === "number" ? r : 0
    ) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    public static fromHex(hexString: string): Color {
        const sliced = hexString.startsWith("#")
            ? hexString.slice(1)
            : hexString;
        const [r, g, b] = sliced.match(/.{2}/g)!;
        return new Color(parseInt(r, 16), parseInt(g, 16), parseInt(b, 16));
    }

    public static fromHSV(
        hue: number,
        saturation: number,
        value: number
    ): Color {
        const { r, g, b } = HSVtoRGB(hue, saturation, value);
        return new Color(r, g, b);
    }

    public static isSame(color1: Color, color2: Color): boolean {
        return (
            color1.r === color2.r &&
            color1.b === color2.b &&
            color1.g === color2.g
        );
    }

    private _colorToHex(color: number) {
        const num = color.toString(16);
        if (num.length === 1) {
            return `0${num}`;
        }
        return num;
    }

    public toJSON(): IColor {
        return {
            r: this.r,
            g: this.g,
            b: this.b,
        };
    }

    public toJSONArray(): number[] {
        return [this.r, this.g, this.b];
    }

    public clone(): Color {
        return new Color(this.r, this.g, this.b);
    }

    public toBytes(): number[] {
        return [this.r, this.g, this.b];
    }

    public isSame(color: Color): boolean {
        return Color.isSame(this, color);
    }

    public toHex(): string {
        return `#${this._colorToHex(this.r)}${this._colorToHex(
            this.g
        )}${this._colorToHex(this.b)}`;
    }

    public toDecimal(): number {
        return parseInt(this.toHex().slice(1), 16);
    }

    public toHSV(): {
        hue: number;
        saturation: number;
        value: number;
    } {
        const { h, s, v } = RGBToHSV(this.r, this.g, this.b);
        return {
            hue: h,
            saturation: s,
            value: v,
        };
    }
}

function HSVtoRGB(h: number, s: number, v: number) {
    let r: number;
    let g: number;
    let b: number;

    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    const p = v * (1 - s);
    const q = v * (1 - f * s);
    const t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
    }
    return {
        r: Math.round(r! * 255),
        g: Math.round(g! * 255),
        b: Math.round(b! * 255),
    };
}

function RGBToHSV(
    r: number,
    g: number,
    b: number
): {
    h: number;
    s: number;
    v: number;
} {
    const rAbsolute = r / 255;
    const gAbsolute = g / 255;
    const bAbsolute = b / 255;
    const value = Math.max(rAbsolute, gAbsolute, bAbsolute);
    const diff = value - Math.min(rAbsolute, gAbsolute, bAbsolute);
    const diffCalculator = (c: number) => (value - c) / 6 / diff + 1 / 2;

    const percentRoundFn = (num: number) => Math.round(num * 100) / 100;

    let h: number = 0;
    let s: number = 0;
    if (diff === 0) {
        h = s = 0;
    } else {
        s = diff / value;
        const rr = diffCalculator(rAbsolute);
        const gg = diffCalculator(gAbsolute);
        const bb = diffCalculator(bAbsolute);

        if (rAbsolute === value) {
            h = bb - gg;
        } else if (gAbsolute === value) {
            h = 1 / 3 + rr - bb;
        } else if (bAbsolute === value) {
            h = 2 / 3 + gg - rr;
        }
        if (h < 0) {
            h += 1;
        } else if (h > 1) {
            h -= 1;
        }
    }
    return {
        h: Math.round(h * 360),
        s: percentRoundFn(s * 100),
        v: percentRoundFn(value * 100),
    };
}

const BYTE_BITS = 8;
const MAX_BYTE_VAL = Math.pow(2, BYTE_BITS) - 1;

enum MOVING_STATUS {
    OFF = 0,
    FORWARDS = 1,
    BACKWARDS = 2,
}

function flatten<V>(arr: V[][]): V[] {
    const resultArr: V[] = [];
    for (const value of arr) {
        resultArr.push(...value);
    }
    return resultArr;
}

function assert(condition: boolean, message: string) {
    if (!condition) {
        throw new Error(message);
    }
}

function shortToBytes(short: number) {
    return [
        (short & (MAX_BYTE_VAL << BYTE_BITS)) >> BYTE_BITS,
        short & MAX_BYTE_VAL,
    ];
}

class Leds {
    private _leds!: (ColorSequence | SingleColor)[];

    public constructor(private readonly _numLeds: number) {}

    private _assertTotalLeds() {
        assert(
            this._leds.reduce((prev, current) => {
                return prev + current.length;
            }, 0) <= this._numLeds,
            "Number of LEDs exceeds total"
        );
        assert(
            this._leds.reduce((prev, current) => {
                return prev + current.length;
            }, 0) >= this._numLeds,
            "Number of LEDs is lower than total"
        );
    }

    public fillWithColors(colors: Color[]): this {
        const numFullSequences = Math.floor(this._numLeds / colors.length);
        this._leds = [];
        this._leds.push(new ColorSequence(colors, numFullSequences));
        if (numFullSequences * colors.length !== this._numLeds) {
            const remainingColors =
                this._numLeds - numFullSequences * colors.length;
            for (let i = 0; i < remainingColors; i++) {
                this._leds.push(new SingleColor(colors[i]));
            }
        }

        this._assertTotalLeds();

        return this;
    }

    public toSequence(): (ColorSequence | SingleColor)[] {
        return this._leds;
    }
}

interface JSONAble {
    toJSON(): unknown;
}

class MoveData implements JSONAble {
    public static readonly MOVING_STATUS = MOVING_STATUS;

    public constructor(
        _moving: MOVING_STATUS.BACKWARDS | MOVING_STATUS.FORWARDS,
        _movingConfig: {
            jumpSize: number;
            jumpDelay: number;
        },
        _alternateConfig?:
            | {
                  alternate: true;
                  alternateDelay: number;
              }
            | {
                  alternate: false;
              }
    );
    public constructor(_moving: MOVING_STATUS.OFF);
    public constructor(
        private readonly _moving: MOVING_STATUS,
        private _movingConfig: {
            jumpSize: number;
            jumpDelay: number;
        } = {
            jumpSize: 0,
            jumpDelay: 0,
        },
        private _alternateConfig:
            | {
                  alternate: true;
                  alternateDelay: number;
              }
            | {
                  alternate: false;
              } = {
            alternate: false,
        }
    ) {}

    public toBytes(): number[] {
        return [
            this._moving,
            ...shortToBytes(this._movingConfig.jumpSize),
            ...shortToBytes(this._movingConfig.jumpDelay),
            ~~this._alternateConfig.alternate,
            ...shortToBytes(
                this._alternateConfig.alternate
                    ? this._alternateConfig.alternateDelay
                    : 0
            ),
        ];
    }

    public toJSON(): unknown {
        return {
            move_status: this._moving,
            jump_size: this._movingConfig.jumpSize,
            jump_delay: this._movingConfig.jumpDelay,
            alternate: ~~this._alternateConfig.alternate,
            alternate_delay: this._alternateConfig.alternate
                ? this._alternateConfig.alternateDelay
                : 0,
        };
    }
}

class ColorSequence implements JSONAble {
    public colors: Color[];

    public get length(): number {
        return (
            (Array.isArray(this.colors) ? this.colors.length : 1) *
            this.repetitions
        );
    }

    public constructor(
        colors: Color[] | Color,
        public repetitions: number = 1
    ) {
        this.colors = Array.isArray(colors) ? colors : [colors];
    }

    public toBytes(): number[] {
        return [
            ColorType.COLOR_SEQUENCE,
            ...shortToBytes(this.colors.length),
            ...shortToBytes(this.repetitions),
            ...flatten<number>(this.colors.map((color) => color.toBytes())),
        ];
    }

    public toJSON(): unknown {
        return {
            type: ColorType.COLOR_SEQUENCE,
            num_colors: this.colors.length,
            repetitions: this.repetitions,
            colors: this.colors.map((c) => c.toJSONArray()),
        };
    }
}

class TransparentSequence implements JSONAble {
    public constructor(public length: number) {}

    public toBytes(): number[] {
        return [ColorType.TRANSPARENT, ...shortToBytes(this.length)];
    }

    public toJSON(): unknown {
        return {
            type: ColorType.TRANSPARENT,
            color: {
                size: this.length,
            },
        };
    }
}

enum ColorType {
    SINGLE_COLOR = 0,
    COLOR_SEQUENCE = 1,
    RANDOM_COLOR = 2,
    TRANSPARENT = 3,
    REPEAT = 4,
}

class SingleColor implements JSONAble {
    public get length(): number {
        return 1;
    }

    public constructor(public color: Color) {}

    public toBytes(): number[] {
        return [ColorType.SINGLE_COLOR, ...this.color.toBytes()];
    }

    public toJSON(): unknown {
        return {
            type: ColorType.SINGLE_COLOR,
            color: this.color.toJSONArray(),
        };
    }
}

class RandomColor implements JSONAble {
    public constructor(
        public size: number,
        public randomTime: number,
        public randomEveryTime: boolean
    ) {}

    public toBytes(): number[] {
        return [
            ColorType.RANDOM_COLOR,
            ~~this.randomEveryTime,
            ...shortToBytes(this.randomTime),
            ...shortToBytes(this.size),
        ];
    }

    public toJSON(): unknown {
        return {
            type: ColorType.RANDOM_COLOR,
            color: {
                random_every_time: this.randomEveryTime,
                random_time: this.randomTime,
                size: this.size,
            },
        };
    }
}

class Repeat implements JSONAble {
    public constructor(
        public repetitions: number,
        public sequence:
            | SingleColor
            | ColorSequence
            | RandomColor
            | TransparentSequence
    ) {}

    public toBytes(): number[] {
        return [
            ColorType.REPEAT,
            ...shortToBytes(this.repetitions),
            ...this.sequence.toBytes(),
        ];
    }

    public toJSON(): unknown {
        return {
            type: ColorType.REPEAT,
            repetitions: this.repetitions,
            sequence: this.sequence.toJSON(),
        };
    }
}

class LedSpecStep implements JSONAble {
    public moveData: MoveData;
    public background: Color;
    public sequences: (
        | SingleColor
        | ColorSequence
        | RandomColor
        | TransparentSequence
        | Repeat
    )[];

    public constructor(
        {
            background,
            moveData,
            sequences,
        }: {
            moveData: MoveData;
            background: Color;
            sequences: (
                | SingleColor
                | ColorSequence
                | RandomColor
                | Repeat
                | TransparentSequence
            )[];
        },
        public delayUntilNext: number = 0
    ) {
        this.moveData = moveData;
        this.background = background;
        this.sequences = sequences;
    }

    public toBytes(): number[] {
        return [
            ...shortToBytes(this.delayUntilNext),
            ...this.moveData.toBytes(),
            ...this.background.toBytes(),
            ...shortToBytes(
                this.sequences
                    .map((sequence) => {
                        if (sequence instanceof Repeat) {
                            return sequence.repetitions;
                        }
                        return 1;
                    })
                    .reduce((p, c) => p + c, 0)
            ),
            ...flatten(this.sequences.map((sequence) => sequence.toBytes())),
        ];
    }

    public toJSON(): unknown {
        return {
            delay_until_next: this.delayUntilNext,
            move_data: this.moveData.toJSON(),
            background: this.background.toJSONArray(),
            num_sequences: this.sequences
                .map((sequence) => {
                    if (sequence instanceof Repeat) {
                        return sequence.repetitions;
                    }
                    return 1;
                })
                .reduce((p, c) => p + c, 0),
            sequences: this.sequences.map((s) => s.toJSON()),
        };
    }
}

class LedEffect implements JSONAble {
    public constructor(public effect: LedSpecStep[]) {}

    public toJSON(): unknown {
        return {
            num_steps: this.effect.length,
            steps: this.effect.map((e) => e.toJSON()),
        };
    }
}

class LedSpec {
    public constructor(public steps: LedEffect) {}

    public toJSON(): unknown {
        return this.steps.toJSON();
    }
}

function scale8(input: number, scale: number = input) {
    return Math.round(input * (scale / 256));
}

function fadeToBlack(
    color: Color,
    steps: number,
    {
        start = true,
        end = true,
    }: {
        start?: boolean;
        end?: boolean;
    } = {}
) {
    const stops: Color[] = [];
    if (start) {
        stops.push(color);
    }

    const delta = 256 / steps;
    for (let i = 1; i < steps - 1; i++) {
        const progress = delta * (steps - i);
        stops.push(
            new Color(scale8(progress), scale8(progress), scale8(progress))
        );
    }

    if (end) {
        stops.push(new Color(0, 0, 0));
    }
    return stops;
}

function interpolate(
    c1: Color,
    c2: Color,
    steps: number,
    {
        start = true,
        end = true,
    }: {
        start?: boolean;
        end?: boolean;
    } = {}
) {
    const stops: Color[] = [];
    if (start) {
        stops.push(c1);
    }

    const delta = 1 / steps;
    for (let i = 1; i < steps - 1; i++) {
        const progress = delta * i;
        const invertedProgress = 1 - progress;
        stops.push(
            new Color(
                Math.round(invertedProgress * c1.r + progress * c2.r),
                Math.round(invertedProgress * c1.g + progress * c2.g),
                Math.round(invertedProgress * c1.b + progress * c2.b)
            )
        );
    }

    if (end) {
        stops.push(c2);
    }
    return stops;
}

const NUM_LEDS = 900;
const rainbow = new LedEffect([
    new LedSpecStep({
        moveData: new MoveData(MOVING_STATUS.OFF),
        background: new Color(255, 0, 0),
        sequences: [],
    }),
    new LedSpecStep({
        moveData: new MoveData(MOVING_STATUS.OFF),
        background: new Color(0, 255, 0),
        sequences: [],
    }),
    new LedSpecStep({
        moveData: new MoveData(MOVING_STATUS.OFF),
        background: new Color(0, 0, 255),
        sequences: [],
    }),
]);

console.log(JSON.stringify(rainbow.toJSON()));
