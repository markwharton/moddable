export class JSONParser {}

export class TreeNode extends Array<any> {
    constructor(type: any);
    type: any;
    get value(): any;
}

export class TreeVPT {
    root: TreeNode;
    stack: TreeNode[];
    initialize(): void;
    makeNode(nodeType: any): TreeNode;
    pop(nodeType: any): boolean;
    node: TreeNode;
    push(nodeType: any): void;
    setText(text: any): void;
    terminate(): void;
}

export { JSONParser as JSONTreeParser };
