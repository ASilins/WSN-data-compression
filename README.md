# Wireless Sensor Network - Data compression mini-project

This is an Aarhus University Wireless Sensor Networks course semesters mini-project. The goal for the project is write lossy or lossless compression algorithm and transmit the data to the sink where the data is reconstructed. The energy consumption is then analysed.

## Cloning the Repository

To get started with the project locally, it is recommended to clone the repository as a submodule for contiki-ng repository locally.

### Recommended steps:
1. Navigate to root of `Contiki-ng` repository
2. Add the submodule and navigate into the project directory:
    ```sh
    git submodule add https://github.com/ASilins/WSN-data-compression.git
    cd WSN-data-compression
    ```
3. Now you're ready to work with the project.

> :information_source: **Note:** \
The project is setup with the assumption that the submodule is pulled in the root of the `Contiki-ng` project. If setup differently, change contiki location in `Makefile`

## Mote setup
To run the motes you have to either run on Cooja or on two motes (on two different computers). On one mote you have to upload producer code, and on the other the sink code. Keep in mind that the motes should be in clear line of sight, and not too far away from each other, otherwise the packet loss will be much bigger.

Build using these commands:

1. Producer:

```sh
    make TARGET=sky producer.upload
   ```

2. Root:

```sh
    make TARGET=sky CLASS=root root.upload
   ```

The motes have two optional build configurations, which you can add to build:
1. CLASS - Defines if you're building producer or root, the default being *producer*. Simple example:
```sh
    make TARGET=sky CLASS=root root.upload
   ```
1. ALGO - Defines which algorithm you're using, default is *sprintz*. Simple example:
```sh
    make TARGET=sky ALGO=sprintz root.upload
   ```

Algorithm options:
1. sprintz - Lossless compression algorithm
2. none - No compression algorithm

### Practical instructions

To run the pipeline, you have to run the producer first, and the sink right after that. Once they're running press the *reset* (red) button on both of them, so the sink can reach the producer - should take around 30 seconds. When it says "Sink reachable" begin sending the data by pressing the *user* (white) button. This starts the pipeline, and you can see the encoded packets being sent on the producer side, and gradually the sink receiving the decoded blocks.

**Optionally, reset the pipeline on both motes, if there are a lot of packets being lost - as indicated in the logs on the sink.**

## Creating new algorithm
To create a new algorithm you have to make the algorithm code in its own directory. In compression directory update encoder and decoder following the Sprintz algorithms implementation example. After that, update the Makefile. First add the general build files needed for the algorithm and then add the decode and encode files based on if you're building the sink or producer.

## Commit messages

It is recommended to follow the following structure for commit messages:
- `add: <commit message>` - used when adding new code/files
- `fix: <commit message>` - used for bug fixes
- `update: <commit message>` - used for update, changes or code optimisation
- `remove: <commit message>` - used for removing code/files
- `refactor: <commit message>` - used when refactoring the code
- `doc: <commit message>` - for documentation changes
- `style: <commit message>` - for formatting, white-space, or style changes

If commit contains more changes and it is hard to group into one, choose on that best describes the changes with the appropriate commit message.

#### Example:
```sh
git commit -m "add: lossy compression algorithm"
```

## Pull Requests

To be able to merge code into develop branch they have to be merged from a Pull Request.

To do that, first create a branch from the develop branch following similar naming strategy as for commit messages:
- `add/<branch_name>`
- `fix/<branch_name>`
- `update/<branch_name>`
- `remove/<branch_name>`
- `refactor/<branch_name>`
- `doc/<branch_name>`
- `style/<branch_name>`

When code changes have been made, push the branch to remote and create a pull request. It should be possible to merge after the Pull Request is approved by at least one person.