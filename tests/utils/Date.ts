export const toISOString = (date: Date) => {
    // The testing framework always set the mills to 000 and doesn't have the Z
    console.log('DEBUG: Writing the raw date', date.toISOString());
    return date.toISOString().split('.')[0] + '.000';
};
