#import <Cocoa/Cocoa.h>
#import <string.h>
#import <stdlib.h>
#import <stdbool.h>
#import "diceparse.h"
#define auto __auto_type
NSWindow* theWindow;
NSTextField* theTextField;
// NSTextView* theTextView;
NSScrollView* scroller;
NSMutableArray<NSString*>* previous_rolls;
long long roll_index;
id<NSTextFieldDelegate> theTextDelegate;
@class RollHistory;
RollHistory* theDataSource;
NSTableView* theTable;

static void do_menus(void);

#define EXPRIDENT @"DiceExpression"
#define DICEROLLIDENT @"DieRoll"
#define TOTALIDENT @"Total"

@interface RollHistory: NSObject <NSTableViewDataSource>
-(void)add_string:(NSString*)s;
@end

@interface DiceRollDelegate: NSObject<NSApplicationDelegate>
@end

@interface RollViewController: NSViewController
-(instancetype)initWithRect:(NSRect)rect;
@end

@interface TextDelegate: NSObject <NSTextFieldDelegate>
@end
int
main(int argc, const char** argv){
    auto ap = [NSApplication sharedApplication];
    auto delegate = [DiceRollDelegate new];
    ap.delegate = delegate;
    return NSApplicationMain(argc, argv);
}

@implementation DiceRollDelegate
-(void)applicationWillFinishLaunching:(NSNotification *)notification{
    previous_rolls = [[NSMutableArray alloc] init];
    do_menus();
}
-(void)applicationDidFinishLaunching:(NSNotification *)notification{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    // auto screen = [NSScreen mainScreen];
    // NSRect rect = screen? screen.visibleFrame : NSMakeRect(0, 0, 1400, 800);
    NSRect rect = NSMakeRect(300, 300, 500, 500);

    theWindow = [[NSWindow alloc]
        initWithContentRect: rect
        styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
        backing: NSBackingStoreBuffered
        defer: NO];
    theWindow.title = @"DiceRoller";
    theWindow.contentViewController = [[RollViewController alloc] initWithRect: rect];
    [theWindow makeKeyAndOrderFront: nil];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{
    return YES;
}
@end

@implementation RollViewController
-(instancetype)initWithRect:(NSRect)rect{
    self = [super init];
    auto split_view = [[NSSplitView alloc] initWithFrame:rect];
    split_view.vertical = NO;
    NSRect tableRect = rect;
    tableRect.size.height -= 200;
    tableRect.origin.y += 200;
    theTable = [[NSTableView alloc] initWithFrame:tableRect];
    NSTableColumn* col;
    col = [[NSTableColumn alloc] initWithIdentifier:EXPRIDENT];
    col.title = @"Expr";
    [theTable addTableColumn:col];
    col = [[NSTableColumn alloc] initWithIdentifier:DICEROLLIDENT];
    col.title = @"Roll";
    [theTable addTableColumn:col];
    col = [[NSTableColumn alloc] initWithIdentifier:TOTALIDENT];
    col.title = @"Total";
    [theTable addTableColumn:col];

    [theTable setHeaderView:nil];

    theTable.columnAutoresizingStyle = NSTableViewUniformColumnAutoresizingStyle;

    theDataSource = [[RollHistory alloc] init];
    theTable.dataSource = theDataSource;
    // theTextView = [[NSTextView alloc] initWithFrame:rect];
    // theTextView.editable = NO;
    NSRect theTextRect = rect;
    theTextRect.size.height = 200;
    theTextField = [[NSTextField alloc] initWithFrame:theTextRect];
    // theTextField = [NSTextField textFieldWithString:@"d20"];
    theTextField.placeholderString = @"Type dice expression here";
    // theTextField.maxSize = NSMakeSize(1e9, 100);
    scroller = [[NSScrollView alloc] initWithFrame:rect];
    scroller.hasVerticalScroller = YES;
    scroller.hasHorizontalScroller = NO;
    // scroller.autoresizingMask = NSViewHeightSizable | NSViewMinXMargin;
    scroller.documentView = theTable;
    // scroller.findBarPosition = NSScrollViewFindBarPositionAboveContent;

    // [split_view addSubview:theTable];
    [split_view addSubview:scroller];
    [split_view addSubview:theTextField];
    // [split_view adjustSubviews];
    theTextDelegate = [[TextDelegate alloc] init];
    theTextField.delegate = theTextDelegate;
    // theTextField.target = theTextDelegate;
    // theTextField.action = @selector(enter);
    self.view = split_view;
    return self;
}
@end

int64_t
visit(DiceParseExpr* exprs, DiceParseExpr expr, bool tight, NSString** outs){
    switch((DiceParseExpressionType)expr.type){
        case DICEPARSE_NUMBER:
            *outs = [*outs stringByAppendingFormat:@"%2u", expr.primary];
            return expr.primary;
        case DICEPARSE_DIE:{
            int64_t total = 0;
            if(tight)
                *outs = [*outs stringByAppendingString:@"("];
            for(int i = 0; i < expr.secondary; i++){
                uint32_t num = (arc4random() % expr.primary) + 1;
                if(i != 0){
                    *outs = [*outs stringByAppendingFormat:@" + [%2u]", num];
                }
                else {
                    *outs = [*outs stringByAppendingFormat:@"[%2u]", num];
                }
                total += num;
            }
            if(tight)
                *outs = [*outs stringByAppendingString:@")"];
            return total;
        }
        case DICEPARSE_BINARY:{
            int64_t value = 0;
            bool tight = expr.type2 == DICEPARSE_MULTIPLY || expr.type2 == DICEPARSE_DIVIDE;
            value = visit(exprs, exprs[expr.primary], tight, outs);
            DiceParseExpr rhs = exprs[expr.secondary];
            switch((DiceParseBinOp)expr.type2){
                case DICEPARSE_ADD:
                    *outs = [*outs stringByAppendingString:@" + "];
                    value += visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_SUBTRACT:
                    *outs = [*outs stringByAppendingString:@" - "];
                    value -= visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_MULTIPLY:
                    *outs = [*outs stringByAppendingString:@" * "];
                    value *= visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_DIVIDE:
                    *outs = [*outs stringByAppendingString:@" / "];
                    {
                        int64_t divisor = visit(exprs, rhs, tight, outs);
                        if(divisor)
                            value /= divisor;
                        else
                            value = 0;
                    }
                    return value;
                case DICEPARSE_EQ:
                    *outs = [*outs stringByAppendingString:@" == "];
                    value = value == visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_NOT_EQ:
                    *outs = [*outs stringByAppendingString:@" != "];
                    value = value != visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_LESS:
                    *outs = [*outs stringByAppendingString:@" < "];
                    value = value < visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_LESS_EQ:
                    *outs = [*outs stringByAppendingString:@" <= "];
                    value = value <= visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_GREATER:
                    *outs = [*outs stringByAppendingString:@" > "];
                    value = value > visit(exprs, rhs, tight, outs);
                    return value;
                case DICEPARSE_GREATER_EQ:
                    *outs = [*outs stringByAppendingString:@" >= "];
                    value = value >= visit(exprs, rhs, tight, outs);
                    return value;
            }
        }
        case DICEPARSE_GROUPING:{
            *outs = [*outs stringByAppendingString:@"("];
            auto result = visit(exprs, exprs[expr.primary], false, outs);
            *outs = [*outs stringByAppendingString:@")"];
            return result;
        }
        case DICEPARSE_UNARY:{
            int64_t num;
            switch((DiceParseUnaryOp)expr.type2){
                case DICEPARSE_PLUS:
                    *outs = [*outs stringByAppendingString:@"+"];
                    return visit(exprs, exprs[expr.primary], true, outs);
                case DICEPARSE_NEG:
                    *outs = [*outs stringByAppendingString:@"-"];
                    num = visit(exprs, exprs[expr.primary], true, outs);
                    return -num;
                case DICEPARSE_NOT:
                    *outs = [*outs stringByAppendingString:@"!"];
                    num = visit(exprs, exprs[expr.primary], true, outs);
                    return !num;
            }
        }
    }
}

@implementation TextDelegate
-(void)enter{
    auto s = theTextField.stringValue;
    [theDataSource add_string:s];
    DiceParseExprBuffer buff = {0};
    int index;
    {
        const char* c = [s UTF8String];
        index = diceparse_parse(&buff, (StringView){.text=c, .length = strlen(c)});
    }
    if(index < 0){
        [theDataSource add_string:@"Error when parsing dice"];
        [theDataSource add_string:@""];
        return;
    }
    if(!previous_rolls.count || ![previous_rolls[previous_rolls.count-1] isEqualToString:s]){
        [previous_rolls addObject:s];
    }
    roll_index = previous_rolls.count-1;
    // s = [s stringByAppendingString:@"\n"];
    // theTextView.string  = [theTextView.string stringByAppendingString:s];
    NSString* str = [NSString new];
    int64_t total = visit(buff.exprs, buff.exprs[index], false, &str);
    [theDataSource add_string:str];
    [theDataSource add_string:[[NSString alloc] initWithFormat:@"%lld", total]];
    // str = [str allocstringByAppendingFormat:@" = %lld\n", total];
    // theTextView.string  = [theTextView.string stringByAppendingString:str];
    // [theTextView scrollRangeToVisible: NSMakeRange(theTextView.string.length, 0)];
    [theTable scrollRowToVisible:theTable.numberOfRows - 1];


}
-(BOOL)control:(NSControl *)control
       textView:(NSTextView *)textView
    doCommandBySelector:(SEL)commandSelector{
        if(commandSelector == @selector(moveUp:)){
            if(roll_index > 0){
                auto s = previous_rolls[--roll_index];
                theTextField.stringValue = s;
            }
            return YES;
        }
        if(commandSelector == @selector(moveDown:)){
            if(roll_index +1< (long long)previous_rolls.count){
                auto s = previous_rolls[++roll_index];
                theTextField.stringValue = s;
            }
            return YES;
        }
        if(commandSelector == @selector(insertNewline:)){
            [theTextDelegate performSelector:@selector(enter)];
        }
        return NO;
}

@end

@implementation RollHistory{
    NSMutableArray<NSString*>* data;
}
-(instancetype)init{
    self = [super init];
    data = [[NSMutableArray alloc] init];
    return self;
}
-(NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
    auto result =  data?data.count/3:0;
    return result;

}
- (id)tableView:(NSTableView *)tableView
    objectValueForTableColumn:(NSTableColumn *)tableColumn
    row:(NSInteger)row{
        int colindex = [tableColumn.identifier isEqualTo:EXPRIDENT]?0:[tableColumn.identifier isEqualTo:DICEROLLIDENT]?1:2;
        size_t index = row*3 + colindex;
        return data[index];
    }
-(void)add_string:(NSString*)s{
  [data addObject: s];
  if(data.count % 3 == 0){
      [theTable reloadData];
  }
}
@end

static
void
do_menus(void){
    NSMenu *mainMenu = [[NSMenu alloc] init];
    // Create the main menu bar
    [NSApp setMainMenu:mainMenu];

    {
        NSString *appName = @"DiceRoll";
        // Create the application menu
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];

        // Add menu items
        NSString *title = [@"About " stringByAppendingString:appName];
        [menu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

        [menu addItem:[NSMenuItem separatorItem]];

        [menu addItemWithTitle:@"Preferences…" action:nil keyEquivalent:@","];

        [menu addItem:[NSMenuItem separatorItem]];

        NSMenu* serviceMenu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* menu_item = [menu addItemWithTitle:@"Services" action:nil keyEquivalent:@""];
        [menu_item setSubmenu:serviceMenu];

        [NSApp setServicesMenu:serviceMenu];

        [menu addItem:[NSMenuItem separatorItem]];

        title = [@"Hide " stringByAppendingString:appName];
        [menu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

        menu_item = [menu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
        [menu_item setKeyEquivalentModifierMask:(NSEventModifierFlagOption|NSEventModifierFlagCommand)];

        [menu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

        [menu addItem:[NSMenuItem separatorItem]];

        title = [@"Quit " stringByAppendingString:appName];
        [menu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

        menu_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        [menu_item setSubmenu:menu];
        [[NSApp mainMenu] addItem:menu_item];
    }

    // Create the edit menu
    {
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Edit"];
        [menu addItem:[NSMenuItem separatorItem]];
        [menu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
        [menu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
        [menu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
        [menu addItem:[NSMenuItem separatorItem]];
        [menu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
        [menu addItemWithTitle:@"Format" action:@selector(format_dnd:) keyEquivalent:@"J"];
        NSMenuItem* mi = [[NSMenuItem alloc] initWithTitle:@"Find..." action:@selector(performTextFinderAction:) keyEquivalent:@"f"];
        mi.tag = NSTextFinderActionShowFindInterface;
        [menu addItem:mi];

        mi = [[NSMenuItem alloc] initWithTitle:@"Find Next" action:@selector(performTextFinderAction:) keyEquivalent:@"g"];
        mi.tag = NSTextFinderActionNextMatch;
        [menu addItem:mi];

        mi = [[NSMenuItem alloc] initWithTitle:@"Find Previous" action:@selector(performTextFinderAction:) keyEquivalent:@"G"];
        mi.tag = NSTextFinderActionPreviousMatch;
        [menu addItem:mi];

        NSMenuItem* menu_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        [menu_item setSubmenu:menu];
        [[NSApp mainMenu] addItem:menu_item];
    }
    // Create the view menu
    {
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"View"];
        [menu addItemWithTitle:@"Zoom Out" action:@selector(zoom_out:) keyEquivalent:@"-"];
        [menu addItemWithTitle:@"Zoom In" action:@selector(zoom_in:) keyEquivalent:@"+"];
        [menu addItemWithTitle:@"Actual Size" action:@selector(zoom_normal:) keyEquivalent:@"0"];


        NSMenuItem* menu_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        [menu_item setSubmenu:menu];
        [[NSApp mainMenu] addItem:menu_item];
    }


    // Create the window menu
    {
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Window"];

        [menu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
        [menu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];

        NSMenuItem* menu_item = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
        [menu_item setSubmenu:menu];
        [[NSApp mainMenu] addItem:menu_item];

        [NSApp setWindowsMenu:menu];
    }
    // Create the help menu
    {
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Help"];

        NSMenuItem* menu_item = [[NSMenuItem alloc] initWithTitle:@"Help" action:nil keyEquivalent:@""];
        [menu_item setSubmenu:menu];
        [[NSApp mainMenu] addItem:menu_item];
        [NSApp setHelpMenu:menu];
    }
}
