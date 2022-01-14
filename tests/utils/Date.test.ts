import {toISOString} from "./Date";

describe('Date tests', () => {
    it('toISOString rounds only after 500ms', () => {
        const testDate = new Date('2022-01-14T17:49:16.000Z');

        for (let i = 0; i < 750; ++i) {
            testDate.setMilliseconds(i);
            expect(toISOString(testDate)).toBe('2022-01-14T17:49:16.000');
        }

        for (let i = 750; i < 1000; ++i) {
            testDate.setMilliseconds(i);
            expect(toISOString(testDate)).toBe('2022-01-14T17:49:17.000');
        }
    });
});
