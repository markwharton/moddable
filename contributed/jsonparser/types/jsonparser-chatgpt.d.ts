declare class JSONParser {
    constructor(vpt: VPT);
    initialize(keys: any[], initialStackDepth: number, initialBufferSize: number): void;
    receive(segment: any, start: number, end: number): void;
    terminate(): void;
}

declare namespace JSONParser {
    const failure: number;
    const receive: number;
    const success: number;
}

declare class Matcher {
    constructor(func?: ((vpt: VPT, node: Node) => void) | undefined, init?: ((vpt: VPT) => void) | undefined, term?: ((vpt: VPT) => void) | undefined);
    initialize(vpt: VPT): this;
    match(vpt: VPT, node: Node): void;
    terminate(vpt: VPT): void;
}

declare class Node {
    type: number;
    prev?: Node | undefined;
    next?: Node | undefined;

    constructor(type: number, prev?: Node | undefined);
    readonly data: any;
    up(count?: number, nodeType?: number | undefined, nodeText?: string | undefined): Node | undefined;
}

declare const NodeType: {
    null: number;
    false: number;
    true: number;
    number: number;
    string: number;
    array: number;
    object: number;
    field: number;
    root: number;
};

declare class VPT {
    matcher: Matcher | undefined;
    node: Node;

    constructor(matcher?: Matcher | undefined);
    initialize(): void;
    makeNode(nodeType: number, prev?: Node | undefined): Node;
    pop(nodeType: number): boolean;
    push(nodeType: number): void;
    setText(text: string): void;
    terminate(): void;
}

export { JSONParser, JSONParser as StreamingJSONParser, Matcher, Node, NodeType, VPT };
