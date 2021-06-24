export const toISOString = (date: Date) => {
    // The testing framework always rounds the  millis and doesn't have the Z
    // 22:47:12.867Z becomes 22:47:13
    const copy = new Date(date);
    copy.setSeconds(Math.round(parseFloat(`${copy.getSeconds()}.${copy.getMilliseconds()}`)));
    copy.setMilliseconds(0);
    return copy.toISOString().split('.')[0] + '.000';
};
