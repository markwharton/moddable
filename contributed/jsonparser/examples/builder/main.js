// Copyright 2023 Mark Wharton (mark@jynx.com)
// https://opensource.org/license/mit/

import { JSONParser, NodeType, VPT } from "jsonparser";

let fragment1 = '{"@odata.context":"https://graph.microsoft.com/v1.0/$metadata#Collection(microsoft.graph.scheduleInformation)","value":[{"scheduleId":"rm03_boardroom@example.com","availabilityView":"022222200000000000000200","scheduleItems":[{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":';
let fragment2 = '"2023-12-05T13:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T13:25:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Mark Wharton","location":"RM-03 \u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T14:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T14:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T15:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T15:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \u2013 Boardroom (AU)","isMeeting":true,"isRecurring":fa';
let fragment3 = 'lse,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T23:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T23:01:00.0000000","timeZone":"E. Australia Standard Time"}}],"workingHours":{"daysOfWeek":["monday","tuesday","wednesday","thursday","friday"],"startTime":"08:00:00.0000000","endTime":"17:00:00.0000000","timeZone":{"name":"Pacific Standard Time"}}}]}';

const keys = ["value", "scheduleItems", "status", "start", "end", "dateTime", "error"];

class BuilderVPT extends VPT {
    // TODO: review
    // keyz;
    // mark;

    constructor(keyz) {
        super(); // drop keys
        this.keyz = keyz;
        this.mark = 0;
    }

    pop(nodeType) {
        if (this.mark === 0)
            delete this.node;
        else
            this.mark--;
        return true;
    }

    push(nodeType) {
        if (this.mark === 0)
            this.node = this.makeNode(nodeType);
        else
            this.mark++;
    }

    setText(text) {
        if (this.mark === 0) {
            if (this.node.type === NodeType.field) {
                let want = false;
                if (this.keyz) {
                    if (this.keyz.includes(text))
                        want = true;
                    else {
                        // fields are pushed before the name is known, so we have to check and balance the tree
                        // prune node that was rejected because it failed to match any of the keys
                        this.pop(this.node.type);
                        this.mark++;
                    }
                } else
                    want = true;
                if (want)
                    trace(`${text}\n`);
            }
        }
    }
}

let vpt = new BuilderVPT(keys);
let parser = new JSONParser(vpt);

parser.initialize();

parser.receive(fragment1);
parser.receive(fragment2);
parser.receive(fragment3);

let result = parser.terminate();
if (result === JSONParser.success)
    trace("success!\n");
else
    trace(`result: ${result}\n`);

debugger;
