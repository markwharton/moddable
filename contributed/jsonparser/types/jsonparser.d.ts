export class JSONParser {
    constructor(vpt: any);
    initialize(keys: any, initialStackDepth: any, initialBufferSize: any): void;
    receive(segment: any, start: any, end: any): void;
    terminate(): void;
}
export namespace JSONParser {
    let failure: number;
    let receive: number;
    let success: number;
}
export class Matcher {
    constructor(func?: any, init?: any, term?: any);
    func: any;
    init: any;
    term: any;
    initialize(vpt: any): this;
    match(vpt: any, node: any): void;
    terminate(vpt: any): void;
}
export class Node {
    constructor(type: any, prev?: any);
    type: any;
    prev: any;
    get data(): {};
    $: {};
    up(count?: number, nodeType?: any, nodeText?: any): this;
}
export const NodeType: Readonly<{
    null: 0;
    false: 1;
    true: 2;
    number: 3;
    string: 4;
    array: 5;
    object: 6;
    field: 7;
    root: 8;
}>;
export class VPT {
    constructor(matcher?: any);
    matcher: any;
    node: Node;
    initialize(): any;
    makeNode(nodeType: any, prev?: any): Node;
    pop(nodeType: any): boolean;
    push(nodeType: any): void;
    setText(text: any): void;
    terminate(): void;
}
export { JSONParser as StreamingJSONParser };
