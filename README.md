## 다음은 운영체제 2번째 과제인 FS쿠에 관한 문서이다.

1. File System을 emulator에서 구현

2. Organization
    - Block size: 512 B -> 
        32 KB = 512 B x 64 blocks: 총 64개 블럭
    - inode size: 16 B -> 2개의 블럭 사용 (64개 인덱싱)
        512 B = 16 B x 32 inodes: 한 블럭에 32 개 inodes
    - Super는 1칸, i-BitMap & d-BitMap도 합쳐서 1칸 사용
        Super에는 따로 저장하지 않는다
    
3. inode
    - 구조체 형식: 16 B
        - unsigned int fsize: 파일 전체 사이즈
        - unsigned int blocks: 몇개의 데이터 블럭으로 있는지
        - unsigned int dptr: 각 1개씩 가지고 있음
        - unsigned int iptr: 각 1개씩 가지고 있음
    - 특정 파일에 대해서 access time 정보는 저장하지 않음

    - 하나의 inode로 Data Blocks를 얼마나 사용할 수 있는가
        - dptr: 4 B 
        - iptr: 4 B -> 512 / 4 = 128 dptr 보유
        total: (1 + 128 dptrs) x 512 B (block size) => 66,048 B 사용 가능

4. Directory
    - root 만 정의함: inode number = 2
    - no subdirectory & no . (curr) & no .. (parent)

    - File name: 3 B
        - 소문자 || 대문자 만 가능
    - inum: 1 B
        - 0: no inode <- 사용하지 않을 때
        - 1: bad block (doesn't happen) <- 없다고 가정
    
5. Initialization
    - Super: 전부 0으로 채워져 있음
    - 0번과 1번은 데이터 넣지 않고 i-bmap에서는 1로 표현 (사용중이라고)
    - 2번은 root directory이기 때문에 사용하고 있음
    - d-bmap에서 가장 앞(0번)은 root directory

    - root directory
        - 데이터 블럭에 다 0으로 채워 엔트리를 가득 채워 줌(간단하게 하기 위해서 공간을 다 쓴다고 가정)
        - fsize = 4 * 61 (0,1,2 를 사용중이므로 61개의 파일까지 가능하다고 봄)

6. Command
    >>> fsku <input_file>
    - root directory initializing
    - 이후 input_file에 들어있는 정해진 내용들을 하나씩 실행한다고 생각하면 됨

    - input file 형식 : string -> <file_name> <I/O command> <size>
        - file name: 2개의 알파벳, 중복 없음
        - I/O command:
            - w-write 해당 파일에 정해진 Bytes of Size 만큼 output
                - file name의 첫번째 문자를 write 한다.
                - 해당 파일이 없다? 새로운 파일 생성 후 write 한다.
                - 이미 존재한다? 해당 파일 뒤에 이어서 작성
                - 파일 중간을 수정하지 않을 것이다~
                - 데이터 블럭을 모두 사용한 경우 Fail 발생한다.
            - r: 항상 파일의 첫번째 지점부터 해당 size만큼 읽겠다(offset 구현하지 않음 -> lseek, offset_set, 0 )
            - d: 생성되지 않은 파일은 오류가 발생해야 함.
        - size: d는 size가 없음
    - Screen Output (success / fail)
        - w: nothing / printf("No space\n");
        - r: print contents, min(fsize,size) + "\n" / printf("No such file\n");
        - d: nothing / printf("No such file\n");

7. EOF of input file: input file의 끝을 만나게 되면
    - storage 내부에 모든 내용을 16진수로 출력

8. Test Stage: 부분 점수를 준다. 야호~
    1. Initialization(empty input file)
    2. w commands
    3. w, r commands
    4. w, r, d commands

9. Submission
    - source file: fsku.c 필요하면 fsku.h 까지는 가능
        - 특별한 컴파일 옵션을 제공하는 라이브러리는 사용하지 않음
    - document
        - basic design (3 ~ 5 pages)
        - description for important function format<function name / functionality / Parameter / Return value>