; ModuleID = 'test.c'

@.str = private unnamed_addr constant [10 x i8] c"Hello %d\0A\00", align 1

; Function Attrs: nounwind uwtable
define i32 @main(i32 %argc, i8** %argv) #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i8**, align 8
  %i = alloca i32, align 4
  store i32 0, i32* %1
  store i32 %argc, i32* %2, align 4
  store i8** %argv, i8*** %3, align 8
  %4 = load i32* %2, align 4
  %5 = icmp sge i32 %4, 2
  br i1 %5, label %6, label %7

; <label>:6                                       ; preds = %0
  store i32 1, i32* %i, align 4
  br label %8

; <label>:7                                       ; preds = %0
  store i32 2, i32* %i, align 4
  br label %8

; <label>:8                                       ; preds = %7, %6
  %9 = load i32* %i, align 4
  %10 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([10 x i8]* @.str, i32 0, i32 0), i32 %9)
  %11 = load i32* %i, align 4
  ret i32 %11
}

declare i32 @printf(i8*, ...) #1

